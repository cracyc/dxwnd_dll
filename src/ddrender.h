typedef struct {
		BOOL nonexclusive;
		BOOL windowed;
		LONG upscale_hack_active;
		int bpp;
		BOOL d3d9linear;
		struct { 
			int width; 
			int height; 
			int x; 
			int y; } viewport;
		int width;
		int height;
		LONG clear_screen;
		LONG surface_updated;
		int l_pitch;
		BOOL vhack;
		HWND hwnd;
		LONG palette_updated;
		char *pBits;
} DDRENDERER;
