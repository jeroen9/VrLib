#include <windows.h>
#include "Font.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include FT_GLYPH_H

#include <VrLib/Log.h>
#include <VrLib/gl/Vertex.h>
#include <VrLib/gl/VBO.h>
#include <VrLib/gl/VAO.h>

namespace vrlib
{
	FT_Library  Font::library = NULL;


	Font::Font(const std::string &name)
	{
		fontName = name;

		if (!library)
		{
			int error = FT_Init_FreeType(&library);
			if (error)
				logger << "Error initializing freetype" << Log::newline;
		}
		
		int error = FT_New_Face(library, "c:/windows/fonts/arial.ttf", 0, &face);
		if (error == FT_Err_Unknown_File_Format)
			logger << "the font file could be opened and read, but it appears that its font format is unsupported" << Log::newline;
		else if (error)
			logger << "Cannot open font" << Log::newline;
		error = FT_Set_Char_Size(
			face,    /* handle to face object           */
			0,       /* char_width in 1/64th of points  */
			16 * 64,   /* char_height in 1/64th of points */
			300,     /* horizontal device resolution    */
			300);   /* vertical device resolution      */


	}



	void Font::generate()
	{
	}


	const float scale = 3000.0f;


	int blaMove(FT_Vector* to, Font::Glyph* outline)
	{
		logger << "Move To " << to->x << ", " << to->y << Log::newline;
		outline->vertices.push_back(std::vector<glm::vec2>());
		outline->vertices.back().push_back(glm::vec2(to->x / scale, to->y / scale));
		return 0;
	}

	int bla(FT_Vector* to, Font::Glyph* outline)
	{
		logger << "Line To " << to->x << ", " << to->y << Log::newline;
		outline->vertices.back().push_back(glm::vec2(to->x / scale, to->y / scale));
		return 0;
	}


	int blaConic(FT_Vector* control, FT_Vector* to, Font::Glyph* outline)
	{
		logger << "Conic " << to->x << ", " << to->y << Log::newline;
		outline->vertices.back().push_back(glm::vec2(to->x / scale, to->y / scale));
		return 0;
	}

	int blaCubic(FT_Vector* control1, FT_Vector* control2, FT_Vector* to, Font::Glyph* outline)
	{
		logger << "Cubic " << to->x << ", " << to->y << Log::newline;
		outline->vertices.back().push_back(glm::vec2(to->x / scale, to->y / scale));
		return 0;
	}

	void Font::generateGlyph(char charCode)
	{
		int glyph_index = FT_Get_Char_Index(face, charCode);

		int error = FT_Load_Glyph(
			face,          /* handle to face object */
			glyph_index,   /* glyph index           */
			FT_LOAD_DEFAULT);  /* load flags, see below */

		FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
		FT_Glyph g;
		error = FT_Get_Glyph(face->glyph, &g);
		FT_Outline_Funcs interface_;

		FT_Outline  outline = face->glyph->outline;

		interface_.line_to = (FT_Outline_LineTo_Func)bla;
		interface_.conic_to = (FT_Outline_ConicTo_Func)blaConic;
		interface_.cubic_to = (FT_Outline_CubicTo_Func)blaCubic;
		interface_.move_to = (FT_Outline_MoveTo_Func)blaMove;
		interface_.shift = 0;
		interface_.delta = 0;

		Glyph* newGlyph = new Glyph();
		newGlyph->charCode = charCode;
		newGlyph->advance = glm::vec2(face->glyph->advance.x / scale, face->glyph->advance.y / scale);

		error = FT_Outline_Decompose(&outline, &interface_, newGlyph);
		FT_Done_Glyph((FT_Glyph)g);


		glyphs[charCode] = newGlyph;

	}

	void Font::render(const char *fmt, ...)					// Custom GL "Print" Routine
	{
		float		length = 0;								// Used To Find The Length Of The Text
		char		text[256];								// Holds Our String
		va_list		ap;										// Pointer To List Of Arguments

		if (fmt == NULL)									// If There's No Text
			return;											// Do Nothing

		va_start(ap, fmt);									// Parses The String For Variables
		vsprintf_s(text, 256, fmt, ap);						// And Converts Symbols To Actual Numbers
		va_end(ap);											// Results Are Stored In Text
		int len = strlen(text);


		std::vector<gl::VertexP3N3T2> vertices;
		gl::VertexP3N3T2 v;
		glm::vec3 pos(0, 0, 0);

		for (int i = 0; i < len; i++)
		{
			char c = text[i];
			if (glyphs.find(c) == glyphs.end())
				generateGlyph(c);


			Glyph* g = glyphs[c];
			


			for (std::vector<glm::vec2> line : g->vertices)
			{
				for (int i = 0; i < (int)line.size(); i++)
				{
					int ii = (i + 1) % line.size();
					
					setP3(v, pos + glm::vec3(line[i].x, 0, line[i].y));						vertices.push_back(v);
					setP3(v, pos + glm::vec3(line[ii].x, 0, line[ii].y));					vertices.push_back(v);
					setP3(v, pos + glm::vec3(line[i].x, 0.1f, line[i].y));					vertices.push_back(v);

					setP3(v, pos + glm::vec3(line[ii].x, 0, line[ii].y));					vertices.push_back(v);
					setP3(v, pos + glm::vec3(line[ii].x, 0.1f, line[ii].y));				vertices.push_back(v);
					setP3(v, pos + glm::vec3(line[i].x, 0.1f, line[i].y));					vertices.push_back(v);
				}
			}
			pos += glm::vec3(g->advance.x, 0, g->advance.y);


		}

		gl::VBO<gl::VertexP3N3T2> vbo;
		vbo.bind();
		vbo.setData(vertices.size(), &vertices[0], GL_STATIC_DRAW);
		gl::VAO<gl::VertexP3N3T2> vao(&vbo);

		glDisable(GL_CULL_FACE);
		vao.bind();
		glDrawArrays(GL_TRIANGLES, 0, vertices.size());
		vao.unBind();
		glEnable(GL_CULL_FACE);



	}

	float Font::getLength(const char *fmt, ...)
	{
/*		if (base == 0)
			generate();
		float		length = 0;								// Used To Find The Length Of The Text
		char		text[256];								// Holds Our String
		va_list		ap;										// Pointer To List Of Arguments

		if (fmt == NULL)									// If There's No Text
			return 0;											// Do Nothing

		va_start(ap, fmt);									// Parses The String For Variables
		vsprintf_s(text, 256, fmt, ap);						// And Converts Symbols To Actual Numbers
		va_end(ap);											// Results Are Stored In Text

		for (unsigned int loop = 0; loop < (strlen(text)); loop++)	// Loop To Find Text Length
		{
			length += gmf[text[loop]].gmfCellIncX;			// Increase Length By Each Characters Width
		}
		return length;*/

		return 0;
	}
}