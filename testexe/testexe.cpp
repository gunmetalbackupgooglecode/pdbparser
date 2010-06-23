extern "C" int _stdcall MessageBoxA (void*, const char*, const char*, unsigned);

int globalC = 0;

void a()
{
	globalC++;
}

int b()
{
	return globalC;
}

void d (int j)
{
	globalC = j;
}

void blabla();

void MyEntry()
{
	a();
	d (0);

	MessageBoxA (0, "MyEntry!", 0, (unsigned)b());

	blabla();
}

void blabla()
{
	d(5);
}