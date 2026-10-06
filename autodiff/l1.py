import math
import random

import matplotlib.pyplot as plt

from sklearn.datasets import make_moons
X, y = make_moons(n_samples=25, noise=0.1, random_state=0)

class Value:
    def __init__(self,data,parents={}):
        self.data = data
        self.grad = 0
        self.parents = parents
        self._backward = self.default_backward

    def default_backward(self):
        return

    def __mul__(self,rhs):
        if not isinstance(rhs,Value):
            return self * Value(rhs)
        newval = Value(self.data * rhs.data,{self,rhs})
        def _backward():
            self.grad += newval.grad * rhs.data
            rhs.grad += newval.grad *self.data
        newval._backward = _backward
        return newval

    def __add__(self,rhs):
        if not isinstance(rhs,Value):
            return self + Value(rhs)
        newval = Value(self.data + rhs.data,{self,rhs})
        def _backward():
            self.grad += newval.grad
            rhs.grad += newval.grad
        newval._backward = _backward
        return newval

    def __pow__(self,exp):
        newval = Value(self.data**exp,{self})
        def _backward():
            self.grad += exp * newval.grad * self.data**(exp - 1)
        newval._backward = _backward
        return newval

    def relu(self):
        newval = Value(max(0,self.data),{self})
        def _backward():
            if self.data > 0:
                self.grad += newval.grad
        newval._backward = _backward
        return newval

    def __neg__(self):
        return Value(-1.0) * self

    def __sub__(self,rhs):
        return self + -rhs

    def __truediv__(self,rhs):
        if not isinstance(rhs,Value):
            return self / Value(rhs)

        return self * rhs**(-1)

    def __radd__(self,lhs):
        return self.__add__(lhs)

    def __rmul__(self,lhs):
        return self*Value(lhs)

    def __rsub__(self,lhs):
        return -self+Value(lhs)

    def __rtruediv__(self,lhs):
        return Value(lhs) / self

    def enum_parents(self,added,outs):
        for p in self.parents:
            p.enum_parents(added,outs)
        if (self not in added):
            added.add(self)
            outs.append(self)
    
    def backward(self):
        self.grad = 1.0
        added=set()
        outs=list()
        self.enum_parents(added,outs)

        outs.reverse()

        for obj in outs:
            obj._backward()


def tanh(val):
    newval = Value(math.tanh(val.data),{val})
    def _backward():
        val.grad += -newval.grad*math.cosh(val.data)**(-2)
    newval._backward = _backward
    return newval


class Neuron:
    def __init__(self,n,rel=True):
        self.drift = Value(random.uniform(-1,1))
        self.w = [Value(random.uniform(-1,1)) for _ in range(n)]
        self.rel=rel

    def __call__(self,nums):
        outp = (sum([x*y for x,y in zip(self.w,nums)]) + self.drift)
        return outp.relu() if self.rel else outp

    def parameters(self):
        return self.w + [self.drift]

class Layer:
    def __init__(self,nin,nout,rel=True):
        self.brain = [Neuron(nin,rel) for _ in range(nout)]

    def __call__(self,x):
        return [neur(x) for neur in self.brain]

    def parameters(self):
        return [p for n in self.brain for p in n.parameters()]

class MLP:
    def __init__(self,neurnums):
        self.layers = [Layer(nin,nout,nout!=1) for nin,nout in zip(neurnums,neurnums[1:] + [1])]

    def __call__(self,dat):
        for layer in self.layers:
            dat = layer(dat)
        return dat[0]

    def parameters(self):
        return [p for l in self.layers for p in l.parameters()]

random.seed()

megatronbrain = MLP([2,16,16])

ys = [2*t - 1 for t in y]

dx = 0.05

prev_err = 10000

for i in range(40):
    err = 0

    for a,b in zip(X,ys):
        for p in megatronbrain.parameters():
            p.grad = 0

        loss = (1 - b*megatronbrain(a)).relu()
        err += loss.data

        loss.backward()

        for p in megatronbrain.parameters():
            p.data -= p.grad * dx

    print(err)

"""
points = []
for _ in range(2000):
    x = random.uniform(-1.5, 2.5)
    y = random.uniform(-1, 1.5)
    output = megatronbrain([x, y]).data
    points.append((x, y, output))


xs = [p[0] for p in points]
ys = [p[1] for p in points]
colors = ['red' if p[2] > 0 else 'blue' for p in points]

plt.scatter(xs, ys, c=colors, s=10)
plt.xlabel('x')
plt.ylabel('y')
plt.title('Decision boundary (red = class +1, blue = class -1)')
plt.show()
"""
