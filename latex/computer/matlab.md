## Function
Matlab 中数组下标从1开始, 而不是像很多种编程语言一样的从0 开始.

	function [y1,...,yN] = myfun(x1,...,xM)
The name of the file should match the name of the first function in the file.  
如果是多个返回值, 调用的时候必须用多个变量来接收函数的返回值.  
例如:`function [x,y]=fun(a,b)`, 使用的时候: `[x,y]=fun(2,3)`.

可以通过多个维度来返回多个值, 例如
	
	function y=f(a,b)
		y(1)=a+b;
		y(2)=a-b;
		y(3)=a*b
使用的时候就是`y=f(2,3)`  
参数也可以使用这种方式

## Solve Equation
### solve
exact solution

### fzero
get a numeric solution from the start value

### fsolve
	fsolve('m file', [initial], option)

### dsolve
get the exact solution of differential equation

	dsolve('eq1', 'eq2',...,'initial', 'variables')
`Dy, D2y` represents `y',y''`

If initial condition is not given, it means to get the general solution

### ode
	[t,x]=solver('f', ts, x0, options)

	t: variable
	x: function
	f: the m file for function to solve
	ts: the time interval
	x0: the initial value of x
solver can be `ode45, ode23, ode15s`

The first thing to try is use `ode45`, then try `ode15s`
