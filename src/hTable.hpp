#define HTABLEDELTA 20

class hTable 
{
public:
	hTable();
    //virtual ~hTable();

	BOOL hGet(HMODULE h);
	void hClean();

private:
	HMODULE *hTableBuffer;
	CRITICAL_SECTION hTableLock; 
	int hTableSize;
	int hTableMax;

};