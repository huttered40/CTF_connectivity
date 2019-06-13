#include <ctf.hpp>

#define PLACE_VERTEX (1)

using namespace CTF;

static Semiring<int> MAX_TIMES_SR(0,
                                  [](int a, int b) {
	                                  return std::max(a, b);
                                  },
                                  MPI_MAX,
                                  1,
                                  [](int a, int b) {
	                                  return a * b;
                                  });

class Int64Pair {
public:
	int64_t i1;
	int64_t i2;
	
	Int64Pair(int64_t i1, int64_t i2);
	
	Int64Pair swap();
};

class Graph {
public:
	int numVertices;
	vector<Int64Pair>* edges;
	
	Graph();
	
	Matrix<int>* adjacencyMatrix(World* world, bool sparse = false);
};

void mat_set(Matrix<int>* matrix, Int64Pair index, int value = PLACE_VERTEX);

int mat_get(Matrix<int>* matrix, Int64Pair index);

void vec_set(Vector<int>* vector, int index, int value = PLACE_VERTEX);

int vec_get(Vector<int>* vector, int index);

Matrix<int>* mat_add(Matrix<int>* A, Matrix<int>* B, World* world);

Matrix<int>* mat_I(int dim, World* world);

Matrix<int>* mat_P(Vector<int>* vector, World* world);

bool mat_eq(Matrix<int>* A, Matrix<int>* B);

Vector<int>* anchor(Graph* graph, World* world);
Vector<int>* anchor_matrix(int n, Matrix<int> * A, World* world);

void test_simple(World* w){
	printf("TEST1: SIMPLE GRAPH 6*6\n");
	auto g = new Graph();
	g->numVertices = 6;
	g->edges->emplace_back(0, 1);
	g->edges->emplace_back(2, 4);
	g->edges->emplace_back(4, 3);
	g->edges->emplace_back(3, 5);
	auto A = g->adjacencyMatrix(w);
	A->print_matrix();
	anchor(g, w)->print();
}

void test_disconnected(World *w){
	printf("TEST2: DISCONNECTED 6*6\n");
	auto g = new Graph();
	g->numVertices = 6;
	auto A = g->adjacencyMatrix(w);
	A->print_matrix();
	anchor(g, w)->print();
}

void test_fully_connected(World *w){
	printf("TEST3: FULLY CONNECTED 6*6\n");
	auto g = new Graph();
	g->numVertices = 6;
	for(int i = 0; i < 5; i++){
      for(int j = i + 1; j < 6; j++){
        g->edges->emplace_back(i, j);
      }
    }
    auto A = g->adjacencyMatrix(w);
	A->print_matrix();
	anchor(g, w)->print();
}

void test_random1(World *w){
	printf("TEST4-1: RANDOM 1 6*6\n");
	Matrix<int> * B = new Matrix<int>(6,6,SP,*w,MAX_TIMES_SR);
	B->fill_sp_random(1.0,1.0,0.1);
	B->print_matrix();
	anchor_matrix(6, B, w)->print();
}

void test_random2(World *w){
    printf("TEST4-2: RANDOM 2 10*10\n");
	Matrix<int> * B = new Matrix<int>(10,10,SP|SH,*w,MAX_TIMES_SR);
	B->fill_sp_random(1.0,1.0,0.1);
	B->print_matrix();
	anchor_matrix(10, B, w)->print();
}

void test_6Blocks_simply_connected(World *w){
	printf("TEST5: 6 Blocks of 6*6 simply connected graph\n");
	auto g = new Graph();
	g->numVertices = 36;
	for(int b = 0; b < 6; b++){
      for(int i = 0; i < 5; i++){
       g->edges->emplace_back(b*6+i, b*6+i+1);
      }
    }
    auto A = g->adjacencyMatrix(w);
	A->print_matrix();
	anchor(g, w)->print();
}

void test_6Blocks_fully_connected(World *w){
	printf("TEST6: 6 Blocks of 6*6 fully connected graph\n");
	auto g = new Graph();
	g->numVertices = 36;
	for(int b = 0; b < 6; b++){
      for(int i = 0; i < 5; i++){
        for(int j = i+1; j < 6; j++)
          g->edges->emplace_back(b*6+i, b*6+j);
      }
    }
    auto A = g->adjacencyMatrix(w);
	A->print_matrix();
	anchor(g, w)->print();
}

void test_simple_kronecker(World* w){
  printf("TEST7: KRONECKER GRAPH: 9*9\n");
  auto g = new Graph();
  g->numVertices = 9;
  g->edges->emplace_back(0, 0);
  g->edges->emplace_back(0, 1);
  g->edges->emplace_back(1, 1);
  g->edges->emplace_back(1, 2);
  g->edges->emplace_back(2, 2);
  auto B = g->adjacencyMatrix(w);
  int lens[] = {3,3,3,3};
  auto D = Tensor<int>(4,B->is_sparse,lens);
  D["ijkl"] = (*B)["ik"]*(*B)["jl"];
  auto B2 = new Matrix<int>(9,9,B->is_sparse*SP,*w,*B->sr);
  delete B;
  B = B2;
  Pair<int> * prs;
  int64_t numprs;
  D.get_local_pairs(&numprs, &prs, true);
  B->write(numprs, prs);
  delete [] prs;
  B->print_matrix();
  anchor_matrix(9,B,w)->print();
  delete B;
}

int main(int argc, char** argv) {
	int rank;
	int np;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &np);
	auto w = new World(argc, argv);
	test_simple(w);
    test_disconnected(w);
    test_fully_connected(w);
    test_random1(w);
    test_random2(w);
    test_6Blocks_simply_connected(w);
    test_6Blocks_fully_connected(w);
    test_simple_kronecker(w);
	return 0;
}

Vector<int>* anchor(Graph* graph, World* world) {
	auto n = graph->numVertices;
	auto v = Vector<int>(n, *world, MAX_TIMES_SR);
	for (auto i = 0; i < n; i++) {
		vec_set(&v, i, i);
	}
	auto A = graph->adjacencyMatrix(world);
	auto I = mat_I(n, world);
	auto AI = mat_add(A, I, world);
	
	auto p = new Vector<int>(n, *world, MAX_TIMES_SR);
	(*p)["i"] = (*AI)["ij"] * v["j"];
	auto P = mat_P(p, world);
	auto Prev = new Matrix<int>(n, n, *world, MAX_TIMES_SR);
	
		// ========== ==========
	while (!mat_eq(P, Prev)) {
		// ========== ==========
		
		Prev = P;
		(*p)["i"] = (*AI)["ij"] * v["j"];
		P = mat_P(p, world);
		
		// ========== ==========
		auto PTA = Matrix<int>(n, n, *world, MAX_TIMES_SR);
		PTA["ij"] = (*P)["li"] * (*A)["lj"];
		auto B = Matrix<int>(n, n, *world, MAX_TIMES_SR);
		B["ij"] = PTA["il"] * (*P)["lj"];
		for (int r = 0; r < n; r++) {
			for (int c = 0; c < r; c++) {
				mat_set(&B, Int64Pair(r, c), 0);
			}
		}
		// ========== ==========
		
		(*P)["ij"] = (*P)["il"] * B["lj"];
		(*P)["ij"] = (*P)["il"] * (*P)["lj"];
	}
	auto PI = mat_add(P, I, world);
	(*p)["i"] = (*PI)["ij"] * v["j"];
	free(A);
	free(P);
	free(I);
	free(PI);
	free(AI);
	free(Prev);
	return p;
}

Vector<int>* anchor_matrix(int n, Matrix<int> * A, World* world) {
	auto v = Vector<int>(n, *world, MAX_TIMES_SR);
	for (auto i = 0; i < n; i++) {
		vec_set(&v, i, i);
	}
	auto I = mat_I(n, world);
	auto AI = mat_add(A, I, world);
	
	auto p = new Vector<int>(n, *world, MAX_TIMES_SR);
	(*p)["i"] = (*AI)["ij"] * v["j"];
	auto P = mat_P(p, world);
	auto Prev = new Matrix<int>(n, n, *world, MAX_TIMES_SR);
	
		// ========== ==========
	while (!mat_eq(P, Prev)) {
		// ========== ==========
		
		Prev = P;
		(*p)["i"] = (*AI)["ij"] * v["j"];
		P = mat_P(p, world);
		
		// ========== ==========
		auto PTA = Matrix<int>(n, n, *world, MAX_TIMES_SR);
		PTA["ij"] = (*P)["li"] * (*A)["lj"];
		auto B = Matrix<int>(n, n, *world, MAX_TIMES_SR);
		B["ij"] = PTA["il"] * (*P)["lj"];
		for (int r = 0; r < n; r++) {
			for (int c = 0; c < r; c++) {
				mat_set(&B, Int64Pair(r, c), 0);
			}
		}
		// ========== ==========
		
		(*P)["ij"] = (*P)["il"] * B["lj"];
		(*P)["ij"] = (*P)["il"] * (*P)["lj"];
	}
	auto PI = mat_add(P, I, world);
	(*p)["i"] = (*PI)["ij"] * v["j"];
	free(A);
	free(P);
	free(I);
	free(PI);
	free(AI);
	free(Prev);
	return p;
}

Int64Pair::Int64Pair(int64_t i1, int64_t i2) {
	this->i1 = i1;
	this->i2 = i2;
}

Int64Pair Int64Pair::swap() {
	return {this->i2, this->i1};
}

Matrix<int>* mat_I(int dim, World* world) {
	auto I = new Matrix<int>(dim, dim, *world, MAX_TIMES_SR);
	for (auto i = 0; i < dim; i++) {
		mat_set(I, Int64Pair(i, i), 1);
	}
	return I;
}

Graph::Graph() {
	this->numVertices = 0;
	this->edges = new vector<Int64Pair>();
}

Matrix<int>* Graph::adjacencyMatrix(World* world, bool sparse) {
	auto attr = 0;
	if (sparse) {
		attr = SP;
	}
	auto A = new Matrix<int>(numVertices, numVertices,
	                         attr, *world, MAX_TIMES_SR);
	for (auto edge : *edges) {
		mat_set(A, edge);
		mat_set(A, edge.swap());
	}
	return A;
}

void mat_set(Matrix<int>* matrix, Int64Pair index, int value) {
	int64_t idx[1];
	idx[0] = index.i2 * matrix->nrow + index.i1;
	int fill[1];
	fill[0] = value;
	matrix->write(1, idx, fill);
}

int mat_get(Matrix<int>* matrix, Int64Pair index) {
	auto data = new int[matrix->nrow * matrix->ncol];
	matrix->read_all(data);
	int value = data[index.i2 * matrix->nrow + index.i1];
	free(data);
	return value;
}

void vec_set(Vector<int>* vector, int index, int value) {
	int64_t idx[1];
	idx[0] = index;
	int fill[1];
	fill[0] = value;
	vector->write(1, idx, fill);
}

int vec_get(Vector<int>* vector, int index) {
	auto data = new int[vector->len];
	vector->read_all(data);
	int value = data[index];
	free(data);
	return value;
}

Matrix<int>* mat_P(Vector<int>* vector, World* world) {
	auto n = vector->len;
	auto A = new Matrix<int>(n, n, *world, MAX_TIMES_SR);
	for (auto row = 0; row < n; row++) {
		for (auto col = 0; col < n; col++) {
			if (vec_get(vector, row) == col) {
				mat_set(A, Int64Pair(row, col), 1);
			} else {
				mat_set(A, Int64Pair(row, col), 0);
			}
		}
	}
	return A;
}

Matrix<int>* mat_add(Matrix<int>* A, Matrix<int>* B, World* world) {
	int n = A->nrow;
	int m = A->ncol;
	auto C = new Matrix<int>(n, m, *world, MAX_TIMES_SR);
	for (auto row = 0; row < n; row++) {
		for (auto col = 0; col < m; col++) {
			auto idx = Int64Pair(row, col);
			auto aVal = mat_get(A, idx);
			auto bVal = mat_get(B, idx);
			mat_set(C, idx, min(1, aVal + bVal));
		}
	}
	return C;
}

bool mat_eq(Matrix<int>* A, Matrix<int>* B) {
	for (int r = 0; r < A->nrow; r++) {
		for (int c = 0; c < A->ncol; c++) {
			if (mat_get(A, Int64Pair(r, c)) != mat_get(B, Int64Pair(r, c))) {
				return false;
			}
		}
	}
	return true;
}
