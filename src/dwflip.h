#ifndef MAXFLIPCHAIN
#define MAXFLIPCHAIN 5

class dxwFlipChain
{
public:
	dxwFlipChain(char *);
	void Initialize(int, LPDIRECTDRAWSURFACE, BOOL);
	void ReleaseFlipChain();

	BOOL AddFlipSurface(LPDIRECTDRAWSURFACE);
	BOOL GetAttachedSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE *);
	//LPDIRECTDRAWSURFACE GetFlipSurface();
	LPDIRECTDRAWSURFACE GetPrimarySurface();
	BOOL SetPrimarySurface(LPDIRECTDRAWSURFACE);
	int GetLength();
	//LPDIRECTDRAWSURFACE GetFlippedSurface(LPDIRECTDRAWSURFACE);
	LPDIRECTDRAWSURFACE GetNext(LPDIRECTDRAWSURFACE);
	void Flip();
	void FlipPrimary(DWORD);
	void Flip(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
	void FlipPrimary(DWORD, LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);

private:
	void dumpFlipChain();
	int version;
	int index;
	int length;
	char *label;
	BOOL cycle;
	DWORD dwSize;
	LPDIRECTDRAWSURFACE flipChain[MAXFLIPCHAIN];
	GetSurfaceDesc2_Type pGetSurfaceDesc;
	Blt_Type pBlt;
};
#endif

