package graph

import algebra.INT_MAX_TIMES_SEMIRING
import algebra.Matrix
import algebra.Vector

private val SR = INT_MAX_TIMES_SEMIRING

fun main() {
	val G = Graph(11, listOf(
			5 to 4,
			4 to 1,
			4 to 3,
			3 to 2,
			11 to 9,
			10 to 9,
			9 to 6,
			9 to 8,
			6 to 8)
	)
	G.superVertex().prettyPrintln()
}

fun Graph.superVertex(A: Matrix<Vertex> = adjacencyMatrix(SR), // adjacency matrix of G
                      p: Vector<Vertex> = verticesVector(SR)) // p[i] = i
		: Vector<Vertex> {
	// update parent by one step
	// here is the naive algorithm update
	// can do one step of other methods as well
	val q = p + A * p
	// preferably, the update deploys sparsity to speed up on recursive calls

	if (q == p) {
		// converged
		return q
	} else {
		val P = q.toParentMatrix() // P[i, j] = 1 <=> q[i] = j
		// transform child edges to parents
		// this step does not shrink matrix in size but introduces more zeros
		val rec_A = P.transpose() * A * P
		val rec_p = superVertex(rec_A, q) // recurse on super vertices
		return Vector(p.length, SR) { rec_p[q[it]] } // set p[i] = rec_p[q[i]]
	}
}
