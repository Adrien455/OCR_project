#ifndef MLP_H
#define MLP_H

typedef struct {
    int in, hid;
    double *W1; // hid x in
    double *b1; // hid
    double *W2; // 1 x hid
    double b2; // scalar
} MLP;

MLP *mlp_create(int in, int hid);
void mlp_free(MLP *m);

double mlp_forward(const MLP *m, const double *x, double *h_buf);
void mlp_backward(MLP *m, const double *x, const double *h, double y, double t, double lr);

#endif