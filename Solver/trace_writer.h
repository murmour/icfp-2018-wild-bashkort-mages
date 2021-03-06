#pragma once

#include "common.h"
#include "zlib.h"


inline int sign(int x)
{
	if (x < 0) return -1;
	if (x == 0) return 0;
	return 1;
}

struct Point
{
	int x, y, z;

	Point to(const Point &other) const
	{
		return { other.x - x, other.y - y, other.z - z };
	}

	bool is_near(const Point &other) const
	{
		int a = Abs(x - other.x), b = Abs(y - other.y), c = Abs(z - other.z);
		int s = a + b + c;
		return a <= 1 && b <= 1 && c <= 1 && s <= 2 && s > 0;
	}

	int mlen() const // manhattan length
	{
		return Abs(x) + Abs(y) + Abs(z);
	}

	int nz_count() const
	{
		return (x != 0) + (y != 0) + (z != 0);
	}

	bool is_fd() const
	{
		return *this != Point::Origin && Abs(x) <= 30 && Abs(y) <= 30 && Abs(z) <= 30;
	}

	Point dir_to(const Point &other) const
	{
		return { sign(other.x - x), sign(other.y - y), sign(other.z - z) };
	}

	bool operator != (const Point &other) const
	{
		return x != other.x || y != other.y || z != other.z;
	}

	bool operator == (const Point &other) const
	{
		return x == other.x && y == other.y && z == other.z;
	}

	bool operator < (const Point &other) const
	{
		if (x != other.x) return x < other.x;
		if (y != other.y) return y < other.y;
		return z < other.z;
	}

	Point operator + (const Point &other) const
	{
		return { x + other.x, y + other.y, z + other.z };
	}

	Point operator * (int k) const
	{
		return { x * k, y * k, z * k };
	}

	Point operator - (const Point &other) const
	{
		return { x - other.x, y - other.y, z - other.z };
	}

	int n_diff(const Point &other) const
	{
		return (x != other.x) + (y != other.y) + (z != other.z);
	}

	static const Point Origin;
};

extern const Point kDeltas6[6];

const std::vector<Point>& Deltas26();

struct Region
{
	Region(const Point &pa, const Point &pb)
	{
		a.x = std::min(pa.x, pb.x);
		a.y = std::min(pa.y, pb.y);
		a.z = std::min(pa.z, pb.z);
		b.x = std::max(pa.x, pb.x);
		b.y = std::max(pa.y, pb.y);
		b.z = std::max(pa.z, pb.z);
	}

	Point opposite(const Point &p)
	{
		auto f = [](int a, int b, int c)
		{
			if (c == a) return b;
			if (c == b) return a;
			Assert(false);
			return -1;
		};

		return { f(a.x, b.x, p.x), f(a.y, b.y, p.y), f(a.z, b.z, p.z) };
	}

	bool contains(const Point &p) const
	{
		return a.x <= p.x && p.x <= b.x &&
			a.y <= p.y && p.y <= b.y &&
			a.z <= p.z && p.z <= b.z;
	}

	bool operator < (const Region &other) const
	{
		if (a != other.a) return a < other.a;
		return b < other.b;
	}

	template<typename F> void for_each(F f) const
	{
		for (int x = a.x; x <= b.x; x++)
			for (int y = a.y; y <= b.y; y++)
				for (int z = a.z; z <= b.z; z++)
					f({ x, y, z });
	}

	int get_dim() const
	{
		return a.n_diff(b);
	}

	int get_bots() const
	{
		return 1 << get_dim();
	}

	Point a, b;
};

constexpr const int kMaxBots = 40;

const int kMaxR = 250;

struct IntM
{
	int m[kMaxR][kMaxR][kMaxR];
};

inline Region get_region(Point p, int s)
{
	Point base = { p.x * s, p.y * s, p.z * s };
	Point opp = { p.x * s + s - 1, p.y * s + s - 1, p.z * s + s - 1 };
	return Region(base, opp);
}

struct Matrix
{
	int R;
	char m[kMaxR][kMaxR][kMaxR];
	IntM *sums = nullptr;
	int XL = -1, XR = -1, ZL = -1, ZR = -1;

	bool load_from_file(const char * filename);
	void clear(int r);

	void init_sums();
	void dump(const char * fname, const std::vector<Point> &bots) const;

	char& operator [] (const Point &p)
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		return m[p.x][p.y][p.z];
	}

	bool get(const Point &p) const
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		if (XL != -1 && (p.x < XL || p.x > XR)) return true;
		if (ZL != -1 && (p.z < ZL || p.z > ZR)) return true;
		return m[p.x][p.y][p.z];
	}

	char operator [] (const Point &p) const
	{
#ifdef DEBUG
		Assert(is_valid(p));
#endif
		return m[p.x][p.y][p.z];
	}

	bool is_valid(const Point &p) const
	{
		return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < R && p.y < R && p.z < R;
	}

	int get_sum(const Region &r) const
	{
		Assert(sums);
		int X = r.b.x, Y = r.b.y, Z = r.b.z;
		int x = r.a.x - 1, y = r.a.y - 1, z = r.a.z - 1;

		int res = sums->m[X][Y][Z];
		if (x > 0)
		{
			res -= sums->m[x][Y][Z];
			if (y > 0)
			{
				res += sums->m[x][y][Z];
				if (z > 0)
					res -= sums->m[x][y][z];
			}
			if (z > 0)
			{
				res += sums->m[x][Y][z];
			}
		}
		if (y > 0)
		{
			res -= sums->m[X][y][Z];
			if (z > 0)
			{
				res += sums->m[X][y][z];
			}
		}
		if (z > 0)
		{
			res -= sums->m[X][Y][z];
		}
		return res;
	}

	bool check_b(const Point &p, int s) const
	{
		Region r = get_region(p, s);
		return get_sum(r) == s * s * s;
	}

	bool is_valid(const Point &p, int s) const
	{
		int N = R / s;
		return p.x >= 0 && p.y >= 0 && p.z >= 0 && p.x < N && p.y < N && p.z < N;
	}

	int get_filled_count() const
	{
		int res = 0;
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (m[x][y][z])
						res++;
		return res;
	}

	void set_x_limits(int x1, int x2)
	{
		XL = x1;
		XR = x2;
	}

	void set_z_limits(int z1, int z2)
	{
		ZL = z1;
		ZR = z2;
	}

	bool check_equal(const Matrix &other) const
	{
		if (R != other.R) return false;
		for (int x = 0; x < R; x++)
			for (int y = 0; y < R; y++)
				for (int z = 0; z < R; z++)
					if (bool(m[x][y][z]) != bool(other.m[x][y][z]))
						return false;
		return true;
	}
};

enum CommandType : u8
{
	cmdHalt,
	cmdWait,
	cmdFlip,
	cmdMove,
	cmdMoveR,
	cmdFusionP,
	cmdFusionS,
	cmdFill,
	cmdFission,
	cmdVoid,
	cmdGFill,
	cmdGVoid,
};

struct Command
{
	i8 dx, dy, dz;
	CommandType ty;
	i8 fdx = 0, fdy = 0, fdz = 0;
	i8 _unused = 0;
};

struct TraceWriter
{
	virtual void halt() = 0;
	virtual void wait() = 0;
	virtual void flip() = 0;
	virtual void move(const Point &from, const Point &to, bool reverse_order = false) = 0;
	virtual void fusion_p(const Point &from, const Point &to) = 0;
	virtual void fusion_s(const Point &from, const Point &to) = 0;
	virtual void fill(const Point &from, const Point &to) = 0;
	virtual void fission(const Point &from, const Point &to, int m) = 0;
	virtual Point do_command(const Point &p, Command cmd, int bot_id) = 0;
	virtual bool can_execute(const Command &cmd) = 0;

	virtual void void_(const Point &from, const Point &to) = 0;
	virtual void g_fill(const Point &from, const Point &to, const Point &fd) = 0;
	virtual void g_void(const Point &from, const Point &to, const Point &fd) = 0;

	virtual int get_n_moves() const = 0;
	virtual bool backtrack(int old_moves_count) = 0;
	virtual int get_filled_count() const = 0;
	virtual bool is_filled(const Point &p) = 0;

	virtual ~TraceWriter() {}
};

struct Bot;

struct MemoryTraceWriter : public TraceWriter
{
	MemoryTraceWriter(Bot *bot) : bot(bot) {}
	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);
	Point do_command(const Point &p, Command cmd, int bot_id);

	void void_(const Point &from, const Point &to);
	void g_fill(const Point &from, const Point &to, const Point &fd);
	void g_void(const Point &from, const Point &to, const Point &fd);
	int get_filled_count() const
	{
		Assert(false);
		return 0;
	}
	bool can_execute(const Command &cmd)
	{
		Assert(false);
		return false;
	}

	bool is_filled(const Point &p)
	{
		Assert(false);
		return false;
	}

	int get_n_moves() const
	{
		return (int)commands.size();
	}

	bool backtrack(int old_moves_count);

	std::vector<Command> commands;
	Bot *bot;
	Point p0; // bot's position before the sequence of commands

private:
	void add(const Command &cmd);
};

struct FileTraceWriter : public TraceWriter
{
	FileTraceWriter(const char *fname, int R, Matrix *src = nullptr);
	~FileTraceWriter();

	void halt();
	void wait();
	void flip();
	void move(const Point &from, const Point &to, bool reverse_order = false);
	void fusion_p(const Point &from, const Point &to);
	void fusion_s(const Point &from, const Point &to);
	void fill(const Point &from, const Point &to);
	void fission(const Point &from, const Point &to, int m);

	void void_(const Point &from, const Point &to);
	void g_fill(const Point &from, const Point &to, const Point &fd);
	void g_void(const Point &from, const Point &to, const Point &fd);

	bool is_filled(const Point &p)
	{
		return mat[p];
	}

	Point do_command(const Point &p, Command cmd, int bot_id);
	bool can_execute(const Command &cmd);
	i64 get_energy() const { return energy; }
	int get_filled_count() const { return n_filled; }
	const Matrix& get_matrix() const { return mat; }

	int get_n_moves() const
	{
		return n_moves;
	}

	bool backtrack(int old_moves_count)
	{
		return false;
	}
private:
	void next();
	void invalidate(const Point &p)
	{
		Assert(!mat[p]);
		Assert(!(inv[p] & 1));
		inv[p] = 1;
		inv_v.push_back(p);
	}
	Point curp() const
	{
		return bots[cur_bot].pos;
	}
	void inv_and_copy()
	{
		invalidate(curp());
		bots_next.push_back(bots[cur_bot]);
	}

	gzFile f;
	bool high_harmonics = false;
	int n_bots = 1;
	int n_bots_next = 1; // number of bots in the next move
	int cur_bot = 0;
	i64 energy = 0; // total energy spent
	int n_filled = 0;
	int R; // resolution
	Matrix mat;
	Matrix inv; // invalid cells
	std::vector<Point> inv_v;
	std::map<Region, int> gr_ops; // how many ops were for this region
	int n_long_moves = 0;
	int n_short_moves = 0;
	int n_moves = 0;

	struct XBot
	{
		Point pos;
		i64 seeds;
		int id;
		bool operator < (const XBot &other) const
		{
			return id < other.id;
		}
	};

	std::vector<XBot> bots, bots_next;
};

struct Bot
{
	Point pos;
	i64 seeds;
	int id;
	int parent = -1;
	int step = 0;
	MemoryTraceWriter mw;
	int left = -1, right = -1;

	Bot() : pos(Point::Origin), seeds(0), id(-1), mw(this) {}

	Bot(Point pos, i64 seeds, int id) : pos(pos), seeds(seeds), id(id), mw(this)
	{
		parent = -1;
		step = 0;
	}

	static Bot* Initial()
	{
		constexpr i64 initial_seeds = (1ll << kMaxBots) - 2;
		return new Bot({ 0, 0, 0 }, initial_seeds, 0);
	}

	DISALLOW_COPY_AND_ASSIGN(Bot);
};

// returns the end point
Point reach_cell(Point from, Point to, const Matrix *env, TraceWriter *w, bool exact = false, const Matrix *bad = nullptr);
void reach_cell(Bot *b, Point to, const Matrix *env, TraceWriter *w, bool exact = false, const Matrix *bad = nullptr);

typedef std::function<int(const Matrix *src, const Matrix *target, TraceWriter *writer)> TSolverFun;

inline int high_bit(i64 seeds)
{
	for (int i = kMaxBots - 1; i >= 0; i--) if (seeds & (1ll << i)) return i;
	Assert(false);
	return -1;
}

inline int low_bit(i64 seeds)
{
	for (int i = 0; i < kMaxBots; i++) if (seeds & (1ll << i)) return i;
	Assert(false);
	return -1;
}

inline i64 make_seeds(int a, int b)
{
	return ((1ll << (b + 1)) - 1) ^ ((1ll << a) - 1);
}

template<typename F>
inline bool check_for_all_subdeltas(Point p, F f)
{
	if (p.x && !f({ p.x, 0, 0 })) return false;
	if (p.y && !f({ 0, p.y, 0 })) return false;
	if (p.z && !f({ 0, 0, p.z })) return false;
	if (p.x && p.y & !f({ p.x, p.y, 0 })) return false;
	if (p.x && p.z & !f({ p.x, 0, p.z })) return false;
	if (p.y && p.z & !f({ 0, p.y, p.z })) return false;
	return true;
}

inline int first_changed_coord(const Point &p)
{
	if (p.x) return 0;
	if (p.y) return 1;
	return 2;
}

inline bool need_reverse(int dir1, int dir2)
{
	return first_changed_coord(kDeltas6[dir1]) > first_changed_coord(kDeltas6[dir2]);
}

// clears commands after collecting
void collect_commands(TraceWriter *w, const std::vector<Bot*> &bots);
bool collect_commands_sync(TraceWriter *w, const std::vector<Bot*> &bots);

void RegisterSolver(const std::string id, TSolverFun f);
TSolverFun GetSolver(const std::string id);

#define REG_SOLVER(id, solver) \
	struct _R_##solver { _R_##solver() { RegisterSolver(id, solver); } } _r_##solver
