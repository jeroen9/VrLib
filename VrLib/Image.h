#pragma once

#include <string>

namespace vrlib
{
	class Image
	{
	public:
		int width;
		int height;
		int depth;
		unsigned char* data;

		Image(const std::string &filename);
		Image(int width, int height);
		~Image();
		void unload();

	};
}