#include "pam.h"
#include <iostream>
#include <algorithm>
#include "pbbs-include/random.h"
#include "pbbs-include/get_time.h"

using namespace std;

using point = int;
using par = pair<point,point>;

struct interval_map {
  
  using interval = pair<point, point>;

  struct entry {
    using key_t = point;
    using val_t = point;
    using aug_t = interval;
    static inline bool comp(key_t a, key_t b) { return a < b;}
    static aug_t get_empty() { return interval(0,0);}
    static aug_t from_entry(key_t k, val_t v) { return interval(k,v);}
    static aug_t combine(aug_t a, aug_t b) {
      return (a.second > b.second) ? a : b;}
  };

  using amap = aug_map<entry>;
  amap m;

  interval_map(size_t n) {
    amap::reserve(n); 
  }

  void finish() {
    amap::finish();
  }

  interval_map(interval* A, size_t n) {
    m = amap(A,A+n); }

  bool stab(point p) {
    return (m.aug_left(p).second > p);}

  void insert(interval i) {m.insert(i);}

  vector<interval> report_all(point p) {
    vector<interval> vec;
    amap a = m;
    interval I = a.aug_left(p);
    while (I.second > p) {
      vec.push_back(I);
      a = amap::remove(move(a),I.first); 
      I = a.aug_left(p); }
    return vec; }

  void remove_small(point l) {
    auto f = [&] (interval I) {
      return (I.second - I.first >= l);};
    m = amap::filter(move(m),f); }
};

long str_to_long(char* str) {
  return strtol(str, NULL, 10);
}

int main(int argc, char** argv) {
  size_t n = 100000000;
  if (argc == 1) {
	  cout << "./intervalTree n q r" << endl;
	  cout << "n: number of points" << endl;
	  cout << "q: number of queries" << endl;
	  cout << "r: number of rounds" << endl;
  }
  if (argc > 1) n = str_to_long(argv[1]);
  size_t q_num = n;
  if (argc > 2) q_num = str_to_long(argv[2]);
  size_t rounds = 5;
  if (argc > 3) rounds = str_to_long(argv[3]);

  par *v = pbbs::new_array<par>(n);
  par *vv = pbbs::new_array<par>(n);
  size_t max_size = (((size_t) 1) << 31)-1;

  pbbs::random r = pbbs::default_random;
  parallel_for (size_t i = 0; i < n; i++) {
    point start = r.ith_rand(2*i)%(max_size/2);
    point end = start + r.ith_rand(2*i+1)%(max_size-start);
    v[i] = make_pair(start, end);
  }

  bool* result = new bool[q_num];
  int* queries = new point[q_num];
  parallel_for (size_t i = 0; i < q_num; i++)
    queries[i] = r.ith_rand(6*i)%max_size;
  
  double* build_tm = new double[rounds];
  double* query_tm = new double[rounds];
  const size_t threads = __cilkrts_get_nworkers();
  for (size_t i=0; i < rounds+1; i++) {
    parallel_for (size_t i = 0; i < n; i++) {
      vv[i] = v[i];
    }
    interval_map xtree(n);
    timer t;
    t.start();
    interval_map itree(vv,n);
    double tm = t.stop();
    if (i>0) build_tm[i-1] = tm;

    timer tq;
    tq.start();
    parallel_for (size_t i = 0; i < q_num; i++) 
      result[i] = itree.stab(queries[i]);
    double tm2 = tq.stop();
    if (i>0) query_tm[i-1] = tm2;

    itree.finish();
  }
  
  auto less = [] (double a, double b) {return a < b;};
  pbbs::sample_sort(build_tm,rounds,less);
  pbbs::sample_sort(query_tm,rounds,less);
	
  cout << "interval build"
       << ", threads = " << threads 
       << ", rounds = " << rounds
       << ", n = " << n
       << ", q = " << q_num
       << ", time = " << build_tm[rounds/2] << endl;

  cout << "interval query"
       << ", threads = " << threads 
       << ", rounds = " << rounds
       << ", n = " << n
       << ", q = " << q_num
       << ", time = " << query_tm[rounds/2] << endl;

  return 0;
}
