#include <stdlib.h>
#include <math.h>
#include "mlp.h"

static inline double sigmoid(double z) { return 1.0 / (1.0 + exp(-z)); }

MLP *mlp_create(int in, int hid) 
{
    MLP *m = (MLP *)calloc(1, sizeof(MLP));
    m -> in = in; m -> hid = hid;
    m -> W1 = (double *)malloc(sizeof(double) * hid * in);
    m -> b1 = (double *)calloc(hid, sizeof(double));
    m -> W2 = (double *)malloc(sizeof(double) * hid);
    m -> b2 = 0.0;
    for (int i = 0; i < hid * in; ++i) m -> W1[i] = ((rand()%2000) / 1000.0 - 1.0) * 0.1;
    for (int i = 0; i < hid; ++i) m -> W2[i] = ((rand()%2000) / 1000.0 - 1.0) * 0.1;
    return m;
}

void mlp_free(MLP *m) {
    if (!m) return;
    free(m -> W1); free(m -> b1); free(m -> W2);
    free(m);
}
    
double mlp_forward(const MLP *m, const double *x, double *h) 
{
    for (int j = 0; j < m -> hid; ++j) 
    {
        double z= m -> b1[j];
        for (int i = 0; i < m -> in; ++i) z += m -> W1[j * m -> in + i] * x[i];
        h[j] = sigmoid(z);
    }
    double z = m -> b2;
    for (int j = 0; j < m -> hid; ++j) z += m -> W2[j] * h[j];
    return sigmoid(z);
}

void mlp_backward(MLP *m, const double *x, const double *h, double y, double t, double lr) 
{
    double dldy = (y - t); // derivative for BCE with sigmoid

    // update output weights
    for (int j = 0; j < m -> hid; ++j) 
    {
        m -> W2[j] -= lr * dldy * h[j];
    }
    m -> b2 -= lr * dldy;

    // backpropagate to hidden
    double dh[m -> hid];
    for (int j = 0; j < m -> hid; ++j) 
    {
        double d = m -> W2[j] * dldy;
        dh[j] = d * h[j] * (1.0 - h[j]);
    }

    // update input -> hidden weights
    for (int j = 0; j < m -> hid; ++j) 
    {
        for (int i = 0; i < m -> in; ++i)
        {
            m -> W1[j * m -> in + i] -= lr * dh[j] * x[i];
        }

        m -> b1[j] -= lr * dh[j];
    }
}