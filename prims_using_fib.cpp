#include <bits/stdc++.h>
using namespace std;

struct FibNode {
    int vertex;
    long long key;
    FibNode *parent, *child, *left, *right;
    int degree;
    bool mark;
    FibNode(int v, long long k) : vertex(v), key(k), parent(nullptr), child(nullptr),
        left(this), right(this), degree(0), mark(false) {}
};

struct FibHeap {
    FibNode* minNode;
    int nNodes;

    FibHeap() : minNode(nullptr), nNodes(0) {}

    void insert(FibNode* x) {
        if (!minNode) {
            minNode = x;
        } else {
            // insert into root list
            x->left = minNode;
            x->right = minNode->right;
            minNode->right->left = x;
            minNode->right = x;
            if (x->key < minNode->key) minNode = x;
        }
        nNodes++;
    }

    void link(FibNode* y, FibNode* x) {
        // remove y from root list
        y->left->right = y->right;
        y->right->left = y->left;
        // make y child of x
        y->parent = x;
        if (!x->child) {
            x->child = y;
            y->left = y->right = y;
        } else {
            y->left = x->child;
            y->right = x->child->right;
            x->child->right->left = y;
            x->child->right = y;
        }
        x->degree++;
        y->mark = false;
    }

    void consolidate() {
        int D = 50; // log2(n) upper bound
        vector<FibNode*> A(D, nullptr);

        vector<FibNode*> roots;
        if (minNode) {
            FibNode* x = minNode;
            do {
                roots.push_back(x);
                x = x->right;
            } while (x != minNode);
        }

        for (FibNode* w : roots) {
            FibNode* x = w;
            int d = x->degree;
            while (A[d]) {
                FibNode* y = A[d];
                if (x->key > y->key) swap(x, y);
                link(y, x);
                A[d] = nullptr;
                d++;
            }
            A[d] = x;
        }

        minNode = nullptr;
        for (FibNode* a : A) {
            if (a) {
                if (!minNode) {
                    a->left = a->right = a;
                    minNode = a;
                } else {
                    a->left = minNode;
                    a->right = minNode->right;
                    minNode->right->left = a;
                    minNode->right = a;
                    if (a->key < minNode->key) minNode = a;
                }
            }
        }
    }

    FibNode* extractMin() {
        FibNode* z = minNode;
        if (z) {
            // add children to root list
            if (z->child) {
                vector<FibNode*> children;
                FibNode* c = z->child;
                do {
                    children.push_back(c);
                    c = c->right;
                } while (c != z->child);
                for (FibNode* x : children) {
                    x->parent = nullptr;
                    x->left = minNode;
                    x->right = minNode->right;
                    minNode->right->left = x;
                    minNode->right = x;
                }
            }
            // remove z
            z->left->right = z->right;
            z->right->left = z->left;
            if (z == z->right) {
                minNode = nullptr;
            } else {
                minNode = z->right;
                consolidate();
            }
            nNodes--;
        }
        return z;
    }

    void cut(FibNode* x, FibNode* y) {
        // remove x from child list of y
        if (x->right == x) {
            y->child = nullptr;
        } else {
            x->left->right = x->right;
            x->right->left = x->left;
            if (y->child == x) y->child = x->right;
        }
        y->degree--;
        // add x to root list
        x->parent = nullptr;
        x->left = minNode;
        x->right = minNode->right;
        minNode->right->left = x;
        minNode->right = x;
        x->mark = false;
    }

    void cascadingCut(FibNode* y) {
        FibNode* z = y->parent;
        if (z) {
            if (!y->mark) {
                y->mark = true;
            } else {
                cut(y, z);
                cascadingCut(z);
            }
        }
    }

    void decreaseKey(FibNode* x, long long k) {
        if (k > x->key) return; // invalid
        x->key = k;
        FibNode* y = x->parent;
        if (y && x->key < y->key) {
            cut(x, y);
            cascadingCut(y);
        }
        if (x->key < minNode->key) minNode = x;
    }

    bool empty() { return minNode == nullptr; }
};

// ---- Prim’s using FibHeap ----
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int n, m;
    cin >> n >> m;
    vector<vector<pair<int,long long>>> adj(n);
    for (int i=0;i<m;i++){
        int u,v; long long w;
        cin >> u >> v >> w;
        --u; --v;
        adj[u].push_back({v,w});
        adj[v].push_back({u,w});
    }

    vector<long long> dist(n, LLONG_MAX);
    vector<int> parent(n,-1);
    vector<FibNode*> nodes(n);

    FibHeap H;

    for (int i=0;i<n;i++){
        nodes[i] = new FibNode(i, dist[i]);
        H.insert(nodes[i]);
    }
    // start at 0
    H.decreaseKey(nodes[0],0);
    dist[0]=0;

    long long totalWeight=0;

    while(!H.empty()){
        FibNode* uNode=H.extractMin();
        int u=uNode->vertex;
        long long d=uNode->key;
        totalWeight+= (d==LLONG_MAX?0:d);

        for(auto &edge: adj[u]){
            int v=edge.first;
            long long w=edge.second;
            if(nodes[v] && w<dist[v]){
                dist[v]=w;
                parent[v]=u;
                H.decreaseKey(nodes[v],w);
            }
        }
        // mark removed
        nodes[u]=nullptr;
    }

    cout<<"Total MST weight: "<<totalWeight<<"\n";
    cout<<"Edges in MST:\n";
    for(int i=1;i<n;i++){
        cout<<parent[i]+1<<" "<<i+1<<" "<<dist[i]<<"\n";
    }
    return 0;
}
