#pragma once

class MTRandom
{
	public:
		MTRandom(unsigned int s): init(true), p(0) {
			seed(s);
			state[n] = { 0x0UL };
		}

		unsigned int next();

	private:
		static const int n = 624, m = 397;
		unsigned int state[n];
		int p;
		bool init;

		unsigned int twiddle(unsigned int, unsigned int);
		void gen_state();
		void seed(unsigned int);
};

inline unsigned int MTRandom::twiddle(unsigned int u, unsigned int v)
{
	return (((u & 0x80000000UL) | (v & 0x7FFFFFFFUL)) >> 1) ^ ((v & 1UL) * 0x9908B0DFUL);
}

inline unsigned int MTRandom::next()
{
	if (p == n) gen_state();

	unsigned int x = state[p++];
	x ^= (x >> 11);
	x ^= (x << 7) & 0x9D2C5680UL;
	x ^= (x << 15) & 0xEFC60000UL;

	return x ^ (x >> 18);
}