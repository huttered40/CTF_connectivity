#include <ctf.hpp>
#include <utility>
#include <cfloat>

#define PLACE_VERTEX (1.0)

using namespace CTF;

class IntPair {
public:
  int64_t i1;
  int64_t i2;
  
  IntPair(int64_t i1, int64_t i2);
};

class Graph {
public:
  /**
   * number of vertices
   */
  int n;
  vector<IntPair> edges;
  
  /**
   * @param n number of vertices
   * @param edges list of edges as a pair of two 0-indexed vertices
   */
  Graph(int n, vector<IntPair> edges);
  
  Matrix<float>* adj_mat(World* w, bool sparse = false);
};

Vector<float>* connectivity(Matrix<float> & A);

static Semiring<float> TSR(0.0,
                           [](float a, float b) { return std::max(a, b); },
                           MPI_MAX,
                           1.0,
                           [](float a, float b) { return a * b; });

void test_simple(World w){
  printf("TEST1: SIMPLE GRAPH 6*6\n");
  auto edges = vector<IntPair>();
  //edges.emplace_back(0, 1);
  edges.emplace_back(1, 2);
  edges.emplace_back(3, 4);
  edges.emplace_back(3, 5);
  edges.emplace_back(4, 5);
  auto g = Graph(6, edges);
  auto B = g.adj_mat(&w);
  B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_disconnected(World w){
  printf("TEST2: DISCONNECTED 6*6\n");
  auto edges = vector<IntPair>();
  //edges.emplace_back(0, 1);
  auto g = Graph(6, edges);
  auto B = g.adj_mat(&w);
  B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_fully_connected(World w){
  printf("TEST3: FULLY CONNECTED 6*6\n");
  auto edges = vector<IntPair>();
  for(int i = 0; i < 5; i++){
    for(int j = i + 1; j < 6; j++){
      edges.emplace_back(i, j);
    }
  }
  auto g = Graph(6, edges);
  auto B = g.adj_mat(&w);
  B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_random_1(World w){
  printf("TEST4-1: RANDOM 1 6*6\n");
  Matrix<float> * B = new Matrix<float>(6,6,SP,w,TSR);
    //srand48(B->wrld->rank);
    B->fill_sp_random(1.0,1.0,0.1);
  B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_random_2(World w){
  printf("TEST4-2: RANDOM 2 10*10\n");
  Matrix<float> * B = new Matrix<float>(10,10,SP,w,TSR);
    //srand48(B->wrld->rank);
    B->fill_sp_random(1.0,1.0,0.2);
  B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_6Blocks_simply_connected(World w){
  printf("TEST5: 6 Blocks of 6*6 simply connected graph\n");
  auto edges = vector<IntPair>();
  for(int b = 0; b < 6; b++){
    for(int i = 0; i < 5; i++){
      edges.emplace_back(b*6+i, b*6+i+1);
      //cout << (b*6+i) << " " << (b*6+i+1) << endl;
    }
  }
  auto g = Graph(36, edges);
  cout << "Large Adjancency Matrix 36*36" << endl;
  auto B = g.adj_mat(&w);
  //B->print_matrix();
  connectivity(*B)->print();
  free(B);
}

void test_6Blocks_fully_connected(World w){
  printf("TEST6: 6 Blocks of 6*6 fully connected graph\n");
  auto edges = vector<IntPair>();
  for(int b = 0; b < 6; b++){
    for(int i = 0; i < 5; i++){
      for(int j = i+1; j < 6; j++)
        edges.emplace_back(b*6+i, b*6+j);
    }
  }
  auto g = Graph(36, edges);
  auto B = g.adj_mat(&w);
  //B->print_matrix();
  cout << "Large Adjancency Matrix 36*36" << endl;
  connectivity(*B)->print();
  free(B);
}

void test_simple_kronecker(World w){
  printf("TEST7: SIMPLE KRONECKER GRAPH: 3*3\n");
  auto edges = vector<IntPair>();
  edges.emplace_back(0, 0);
  edges.emplace_back(0, 1);
  edges.emplace_back(1, 1);
  edges.emplace_back(1, 2);
  edges.emplace_back(2, 2);
  auto g = Graph(3, edges);
  auto B = g.adj_mat(&w);
  int lens[] = {3,3,3,3};
  auto D = Tensor<float>(4,B->is_sparse,lens);
  D["ijkl"] = (*B)["ik"]*(*B)["jl"];
  auto B2 = new Matrix<float>(9,9,B->is_sparse*SP,w,*B->sr);
  delete B;
  B = B2;
  Pair<float> * prs;
  int64_t numprs;
  D.get_local_pairs(&numprs, &prs, true);
  B->write(numprs, prs);
  delete [] prs;
  B->print_matrix();
  connectivity(*B)->print();
  delete B;
}


int main(int argc, char** argv) {
  int rank;
  int np;
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &np);
  World w(argc, argv);
  //test_simple(w);
  //test_disconnected(w);
  //test_fully_connected(w);
  //test_random_1(w);
  //test_random_2(w);
  test_6Blocks_simply_connected(w);
  //test_6Blocks_fully_connected(w);
  //test_simple_kronecker(w);
}

IntPair::IntPair(int64_t i1, int64_t i2) {
  this->i1 = i1;
  this->i2 = i2;
}

Graph::Graph(int n, vector<IntPair> edges) {
  this->n = n;
  this->edges = std::move(edges);
}

Matrix<float>* Graph::adj_mat(World* w, bool sparse) {
  auto attr = 0;
  if (sparse) {
    attr = SP;
  }
  auto A = new Matrix<float>(n, n, attr, *w, TSR);
  
  auto m = (int64_t) edges.size();
  auto m2 = m * 2;
  auto idx = new int64_t[m2];
  for (int i = 0; i < m; i++) {
    auto edge = edges[i];
    idx[i] = edge.i1 * n + edge.i2;
    idx[i + m] = edge.i2 * n + edge.i1;
  }
  float fill[m2];
  for (int i = 0; i < m2; i++) {
    fill[i] = PLACE_VERTEX;
  }
  A->write(m2, idx, fill);
  
  free(idx);
  return A;
}

Vector<float>* connectivity(Matrix<float> & A) {
  assert(A.nrow == A.ncol);
  int n = A.nrow;
  
  auto w = new Vector<float>(n, SP, *A.wrld, TSR);
  auto idx = new int64_t[n];
  auto fill = new float[n];
  for (int i = 0; i < n; i++) {
    idx[i] = i;
    fill[i] = i;
  }
  w->write(n, idx, fill);
  free(idx);
  free(fill);
  
  // update adj mat
  for (int i = 0; i < n; i++) {
    (*w)["j"] += A["jk"] * (*w)["k"];
  }
  
  return w;
}
