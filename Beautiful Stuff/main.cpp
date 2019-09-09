#include <iostream>
#include <SDL/SDL.h>
#include <vector>

#pragma comment(lib,"SDL2.lib")
#undef main

struct MyPoint
{
	float r, g, b;
};

void main()
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window* window;
	SDL_Renderer* renderer;
	int w = 1280;
	int h = 720;

	SDL_CreateWindowAndRenderer(w, h, 0, &window, &renderer);

	SDL_Event ev;
	std::vector<MyPoint> points;
	for (int i = 0; i < w*h; i++)
	{
		float r = rand();
		float g = rand();
		float b = rand();
		r /= RAND_MAX;
		g /= RAND_MAX;
		b /= RAND_MAX;
		int x = i % w;
		int y = i / w;

		points.push_back({ r, g, b});
	}
	while (true)
	{
		while (SDL_PollEvent(&ev))
		{

		}
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		
		auto it = points.begin();
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				MyPoint& p = *it++;
				SDL_SetRenderDrawColor(renderer, p.r * 255, p.g * 255, p.b * 255, 255);
				SDL_RenderDrawPoint(renderer, x, y);
			}
		}
		
		SDL_RenderPresent(renderer);
	}

	system("pause");
}