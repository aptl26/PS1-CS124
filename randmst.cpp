#include<iostream>
#include<vector>
#include<cmath>
#include<string>
#include<random>
#include <queue>
#include<thread>
#include <time.h>
#include <bits/stdc++.h>
#include <cassert>

using namespace std;

class Edge{
    public:
        int v;
        int w;
        float weight;

        Edge(int v1, int v2, float edgeweight){
            v = v1;
            w = v2;
            weight = edgeweight;
        }

        Edge(const Edge& t) {
            v = t.v;
            w = t.w;
            weight = t.weight;
        }
};

class EdgeCompareClass {
    public:
        bool operator() (Edge a, Edge b){
        return a.weight > b.weight;
        }
};

// Priority Heap implementation
class MinHeap {
    public:
        Edge e;
        MinHeap* c1 = (MinHeap*) malloc(sizeof(MinHeap));
        MinHeap* c2 = (MinHeap*) malloc(sizeof(MinHeap));
        int size = 1;

        MinHeap (const Edge & inite) : e(inite) {}

        void push(Edge arge) {
            size += 1;
            if (e.v == -1) {
                e.v = arge.v;
                e.w = arge.w;
                e.weight = arge.weight;
                size = 1;
                c1->e.weight = 2.0;
                c2->e.weight = 2.0;
            }
            else if (c1->e.weight == 2.0) {
                *c1 = MinHeap(arge);
                c1->c1->e.weight = 2.0;
                c1->c2->e.weight = 2.0;
                swap();
            }
            else if (c2->e.weight == 2.0) {
                *c2 = MinHeap(arge);
                c2->c1->e.weight = 2.0;
                c2->c2->e.weight = 2.0;
                swap();
            }
            else {
                if (c1->size <= c2->size) {
                    c1->push(arge);
                }
                else {
                    c2->push(arge);
                }
                swap();
            }
        }

        void swap() {
            if (c1->e.weight != 2.0 && e.weight > c1->e.weight &&
                    c2->e.weight != 2.0 && e.weight > c2->e.weight) {
                        if (c1->e.weight <= c2->e.weight) {
                            Edge phe = e;
                            e = c1->e;
                            c1->e = phe;
                            c1->swap();
                        }
                        else {
                            Edge phe = e;
                            e = c2->e;
                            c2->e = phe;
                            c2->swap();
                        }
                    }
            else if (c1->e.weight != 2.0 && e.weight > c1->e.weight) {
                Edge phe = e;
                e = c1->e;
                c1->e = phe;
                c1->swap();
            }
            else if (c2->e.weight != 2.0 && e.weight > c2->e.weight) {
                Edge phe = e;
                e = c2->e;
                c2->e = phe;
                c2->swap();
            }
        }

        Edge pop() {
            Edge phe = e;
            e.weight = 2.0;
            e.v = -1;
            e.w = -1;
            swap();
            return phe;
        }

        bool empty() {
            return e.v == -1;
        }
};

float rand_num() {
    return (float) ((double) rand() / (double) INT_MAX);
};

// Generates N vertices sampled uniformly from R^D
vector<vector<float> > generate_vertex(int N, int D){
    vector<vector<float> > V = {};
    // Generate `N` vertices
    for (int i=0; i<N; i++){
        vector<float> Vi = {};
        for (int j=0; j<D; j++)
            Vi.push_back(rand_num());
        V.push_back(Vi);
    }
    return V;
}

// Calculates adjacency matrix for edges [start, end)
void edgethread(vector<vector<Edge> >& E, const vector<vector<float> >& V, int N, int D, float pruneconst, int start, int end){
    for (int i = start; i < end; i++){
        // cout << i << endl;
        for (int j = 0; j < N; j++){
            if (i == j)
                continue;
            // Calculate distance between V[i] and V[j]
            float dist = 0;
            for (int k=0; k<D; k++){
                // Shortcut: immediately ignore this edge if one component is too large
                if (abs(V[i][k] - V[j][k]) > pruneconst){
                    dist = 1;
                    break;
                }
                dist += pow(V[i][k] - V[j][k], 2);
            }
            dist = pow(dist, .5);
            if (dist < pruneconst){
                Edge eij = Edge(i, j, dist);
                E[i].push_back(eij);
            }
        }
    }
}

vector<vector<Edge> > edges_0(int N, float cutoff) {
    vector<vector<Edge> > E;
    for (int i = 0; i < N; i++) {
        E.push_back({});
    }
    for (int i = 0; i < N; i++) {
        for (int j = i + 1; j < N; j++) {
            float w = rand_num();
            if (w > cutoff) continue;
            Edge edg1 = Edge(i, j, w);
            E[i].push_back(edg1);
            Edge edg2 = Edge(j, i, w);
            E[j].push_back(edg2);
        }
    }
    return E;
}

// Given N vertices sampled uniformly from R^D
// Returns in adjacency-list form: E[i] contains the vector of indices i is connected to
// Prunes all edges of length larger than pruneconst
vector<vector<Edge> > edgefromvertex(const vector<vector<float> >& V, int N, int D, float pruneconst){
    vector<vector<Edge> > E;
    for (int i = 0; i < N; i++)
        E.push_back({});
    
    // Naive implementation without concurrency: edgethread(E, V, N, D, pruneconst, blocklength*i, blocklengthx*(i+1))));

    // Split the workload into `nthreads` blocks
    vector<std::thread> threads;
    int nthreads = max(1, min((int) thread::hardware_concurrency(), (int) (N / 500)));
    int blocklength = N / nthreads;
    for (int i=0; i<nthreads - 1; i++)
        threads.push_back(thread(edgethread, ref(E), ref(V), N, D, pruneconst, blocklength*i, blocklength*(i+1)));
    threads.push_back(thread(edgethread, ref(E), ref(V), N, D, pruneconst, blocklength*(nthreads-1), N));
    for (auto& t : threads)
        t.join();
    return E;
}

// Returns weight of minimum spanning tree, returns -1 upon failure
float prim(vector<vector<Edge> > E, int N){
    MinHeap Q(Edge(-1, -1, 2.));
    vector<Edge> mst;
    bool spanned[N]; // whether index i is connected to MST
    // Initialization: we start with v0
    for (int i=0; i<N; i++)
        spanned[i] = false;
    spanned[0] = true;

    for (auto& e : E[0]) // for e in E[0]
        Q.push(e);

    for (int i=0; i<N-1; i++){
        bool init = true;
        auto minE = Edge(-1, -1, 2.0);
        // Select a minimum-weight edge from current cut. Remove those which are not in cut
        while (init || (spanned[minE.v] && spanned[minE.w])){
            if (Q.empty()) // Returns -1 if Q is empty (ran out of edges, which indicates failure)
                return -1;
            minE = Q.pop();
            init = false;
        }
        int newv = spanned[minE.w] ? minE.v : minE.w;
        // insert newv into spanned vertices
        spanned[newv] = true;
        mst.push_back(minE);
        // Insert new edges into current cut
        for (auto &e : E[newv])
            if (!(spanned[e.w] && spanned[e.v])){
                Q.push(e);
            }
    }
    float mstweight = 0;
    for (auto& e : mst)
        mstweight += e.weight;
    return mstweight;
}


float run_trial(int N, int D){
    auto V = generate_vertex(N, D);

    // sqrt(D/N) is just a heuristic function. We start at this value and increment cutoff upon failure
    float cutoff = pow((float) D, .5) * pow(N, -1. / D) / 2, mstweight = 0;
    bool rerun = true;
    while (rerun) {
        auto E = edgefromvertex(V, N, D, cutoff);
        mstweight = prim(E, N);
        if (mstweight == -1){
            cutoff += pow((float) D, .5) * pow(N, -1. / D) / 2; // Regenerate graph and run again
        }
        else
            rerun = false;
    }
    return mstweight;
}

float run_trial_0 (int N) {
    float cutoff = 100.0 / (float) N;
    float mstweight = 0;

    bool rerun = true;
    while (rerun) {
        auto E = edges_0(N, cutoff);
        // cout << "Edges generated @ cutoff " << cutoff << endl;
        mstweight = prim(E, N);
        if (mstweight == -1){
            cutoff *= 2; // Regenerate graph and run again
        }
        else
            rerun = false;
    }
    return mstweight;
}

int main(int argc, char** argv)
{
    srand(time(0));
    // Parse arguments
    int N = stoi(string(argv[1])), K = stoi(string(argv[2])), D = stoi(string(argv[3]));

    float mstweight = 0;

    for (int i = 0; i < K; i++)
        mstweight += (D == 0) ?  run_trial_0(N) : run_trial(N, D);

    mstweight /= K;
    cout << mstweight << " " << N << " " << K << " " << D << endl;
    return 0;
}