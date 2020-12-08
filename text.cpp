// returns:
// 1 on success
// 0 on failure (bitmap too small)
// -1 on error
static int text_font_load_(State *state, Font *font, u8 const *data,
	float char_height, int bitmap_width, int bitmap_height) {

	u32 mark = tmp_push(state);
	u8 *bitmap = tmp_alloc(state, (size_t)bitmap_width * (size_t)bitmap_height);
	int ret = 1;

	/*
		The number of bytes in a row of a texture must be a multiple of 4.
		See: https://www.khronos.org/opengl/wiki/Common_Mistakes#Texture_upload_and_pixel_reads
	*/
	assert(bitmap_width % 4 == 0);

	font->char_height = char_height;
	font->tex_width = bitmap_width;
	font->tex_height = bitmap_height;

	if (bitmap) {
		int err;
		err = stbtt_BakeFontBitmap(data, 0, char_height, bitmap, bitmap_width, bitmap_height, 32, arr_count(font->char_data),
			font->char_data);
		if (err > 0) {
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, bitmap_width, bitmap_height, 0, GL_ALPHA, GL_UNSIGNED_BYTE, bitmap);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			font->texture = texture;
			debug_println("Loaded font (size = %f), using %d rows of a %dx%d bitmap.",
				char_height, err, bitmap_width, bitmap_height);
		} else {
			// bitmap not big enough 
			ret = 0;
		}
	} else {
		debug_println("Not enough memory for font bitmap.");
		ret = -1;
	}
	tmp_pop(state, mark);
	return ret;
}

static bool text_font_load(State *state, Font *font, char const *filename, float char_height) {
	bool success = false;
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t file_size = (size_t)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		u8 *data = (u8 *)calloc(1, file_size);
		if (data) {
			fread(data, 1, file_size, fp);
			int bitmap_width = 400, bitmap_height = 400;
			while (bitmap_width < 2000) {
				int r = text_font_load_(state, font, data, char_height, bitmap_width, bitmap_height);
				if (r == -1) {
					break;
				} else if (r == 1) {
					success = true;
					break;
				}
				bitmap_width *= 2;
				bitmap_height *= 2;
			}
			free(data);
		} else {
			debug_println("Out of memory.");
		}
		fclose(fp);
	} else {
		debug_println("File not found: %s.", filename);
	}
	if (!success) {
		memset(font, 0, sizeof *font);
	}
	return success;
}

static void text_render_(State *state, Font *font, char const *s, float *xp, float *yp, bool render) {
	if (!font->char_height) return; // font not loaded properly

	stbtt_bakedchar *cdata = font->char_data;
	int tex_w = font->tex_width, tex_h = font->tex_height;
	float x = *xp, y = *yp;
	float widthf = (float)state->win_width, heightf = (float)state->win_height;
	float inv_widthf = 1.0f / widthf, inv_heightf = 1.0f / heightf;

	// convert between GL and stb_truetype coordinates
	x = (x + 1) * 0.5f * widthf;
	y = (1 - (y + 1) * 0.5f) * heightf;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, font->texture);
	glBegin(GL_QUADS);
	for (; *s; ++s) {
		char c = *s;
		if (c >= 32 && (size_t)c < 32+arr_count(font->char_data)) {
			stbtt_aligned_quad q = {};
			stbtt_GetBakedQuad(cdata, tex_w, tex_h, c - 32, &x, &y, &q, 1);
			if (render) {
				q.x0 = (q.x0 * inv_widthf) * 2 - 1;
				q.x1 = (q.x1 * inv_widthf) * 2 - 1;
				q.y0 = (1 - q.y0 * inv_heightf) * 2 - 1;
				q.y1 = (1 - q.y1 * inv_heightf) * 2 - 1;
				{
					// no clipping 
					#if 0
					printf("%f %f %f %f\n",q.s0*tex_w,q.t0*tex_h,q.s1*tex_w,q.t1*tex_h);
					#endif
					glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
					glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
					glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
					glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
				}
			}
		}
	}
	glEnd();
	glDisable(GL_TEXTURE_2D);
	*xp = (x * inv_widthf) * 2 - 1;
	*yp = (1 - y * inv_heightf) * 2 - 1;
}

static void text_render2f(State *state, Font *font, char const *s, float x, float y) {
	text_render_(state, font, s, &x, &y, true);
}
static void text_render(State *state, Font *font, char const *s, v2 pos) {
	text_render_(state, font, s, &pos.x, &pos.y, true);
}

static float text_font_char_height(State *state, Font *font) {
	return font->char_height / (float)state->win_width * 1.3333333f;
}

static v2 text_get_size(State *state, Font *font, char const *s) {
	float x = 0, y = 0;
	text_render_(state, font, s, &x, &y, false);
	return V2(x, y + text_font_char_height(state, font));
}

static void text_render_centered(State *state, Font *font, char const *s, v2 center) {
	text_render(state, font, s, v2_sub(center, v2_scale(text_get_size(state, font, s), 0.5f)));
}

static void text_font_free(Font *font) {
	glDeleteTextures(1, &font->texture);
}
