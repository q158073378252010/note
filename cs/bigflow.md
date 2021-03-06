# Intro
Baidu Bigflow Python是一个Python module,利用module中的数据抽象和API,你可以书写自己的计算任务.
它提供了一套函数式编程风格的API,同时将这些API串联/级联成为`数据流管道(bigflow pipeline)`.
Bigflow Python能够将Pipeline映射成为不同分布式计算引擎上的计算任务,例如(Hadoop/Task Manager/Spark),极大地简化分布式计算程序的编写和维护.

```Python
from bigflow import base, transforms, input, output
pipeline = base.Pipeline.create('local')
```
## 并发和reducer 数量
- 并发可以直接通过hadoop 的参数来指定
- redcuer 数量可以通过 default concurrency 来指定, 默认是1000
- 有些算子, 比如grouy by, join 可以指定局部的concurrency, 此参数优先级高于default concurrency
	- `p.group_by(lambda x: x, concurrency = 1000)`
	- `a.join(b, concurrency = 1000)`
- 如果数据量过小, bigflow 会对concurrency 进行动态缩减, 但是不会在数据很大时动态增加

```Python
pipeline = base.Pipeline.create(
		'hadoop',
		job_name = 'xxx',
		tmp_data_path = 'hdfs://xxx',
		hadoop_job_conf = {
			'mapred.job.map.capacity': '4000',
			'mapred.job.reduce.capacity': '1100'
			},
		default_concurrency = 1000
		)
```

# IO
`input.TextFile(*path, **options)`

- path 可以为本地路径(相对或绝对), 也可以为hadoop集群路径
	- `pipeline.read(input.TextFile('hdfs://host:port/my_hdfs_file'))`
	- `pipeline.read(input.TextFile('hdfs:///multi_path1', 'hdfs:///multi_path2'))`
	- `pipeline.read(input.TextFile(*['hdfs:///multi_path1', 'hdfs:///multi_path2']))`
	- `pipeline.read(input.TextFile('./local_file_by_rel_path/'))`
	- `pipeline.read(input.TextFile('/home/work/local_file_by_abs_path/'))`
- options
	- partitioned: 默认为False，如果置为True，则返回的数据集为一个ptable, ptable的key是split info(类似于hadoop中的`$map_input_file` 环境变量, value这个split上的全部数据所组成的pcollection.

`output.TextFile(path, **options)`

- path 和 input 一样, 可以为本地或集群路径
- 可以串一些选项, 比如sort, partition
	- `sort(reverse=False)`: 根据数据实际值对数据进行排序(默认为升序)
	- `sort_by(key_read_fn=None, reverse=False)`
	- `partition(n=None, partition_fn=None)`: 对输出结果进行分组
	- eg: `output.TextFile("your output dir").sort().partition(n=2000, partition_fn=lambda x, n: hash(x.split("\t", 1)[0]) % n)`

# API
- `pipeline.add_file(path, path, executable=True)`: 经过测试, 两个path 最好一致, 可执行文件, 需要给可执行权限

# P类型(PType)
- PCollection – 并行化的Python list
- PObject – 并行化的Python单个变量
- PTable – 并行化的 Python dict

P类型可能通过下面的三种情况构造:

1. `Pipeline.read()`: 从外部存储读取
1. `Pipeline.parallelize()`: 通过内存变量构造, `pipeline.parallelize([3, 7, 1])`, `pipeline.parallelize(4)`, `pipeline.parallelize([("A", 1), ("A", 2), ("B", 1)]`
1. 从另一个P类型变换得到

P类型可以通过下面的三种情况使用:

1. `Pipeline.write()`: 持久化到外部存储
1. `Pipeline.get()`: 转换为Python内置变量
1. 作用一个 变换 ,生成另一个P类型, P类型的不断变换构造出一个有向无环图,最终完整地表达出用户的计算逻辑.

## PCollection
PCollection表示并行化的Python list .其可以通过一个Python list实例转换得到,或是读取文件得到:

```Python
>>> p1 = pipeline.read(input.TextFile("xxx"))
>>> print p1
[...]
>>> p2 = pipeline.parallelize([1, 2, 3])
>>> print p2
[...]
```
字符串 "[...]" 表示p1/p2均为PCollection.绝大多数的Bigflow变换均作用于PCollection上,而且结果多为PCollection.

PCollection非常类似于Apache Spark中的弹性分布式数据集(RDD).

## PObject
PObject表示单个变量.其往往是聚合类变换的结果,例如max/sum/count等.

```Python
>>> print p1.count()
o
>>> print p2.max()
o
```
字符"o" 表示一个PObject.PObject往往作为一个 SideInput 参与到变换中.具体的例子请参照 SideInput 部分.

## PTable
PTable非常类似于并行化的Python dict, 其包含key到value的映射,但其value必须是另一个P类型.PTable往往是一个分组变换的结果:

```Python
>>> p3 = pipeline.parallelize([("A", 1), ("A", 2), ("B", 1)])
>>> p4 = p3.group_by_key() # group_by_key() 的输入PCollection的所有元素必须是有两个元素的tuple或list.第一个元素为key,第二个元素为value.
>>> print p4
{k0: [...]}
>>> print p4.get()
{"A": [1, 2], "B": [1]}
```
字符串 {k0: [...]} 表示 p4 是一个PTable. p4 的key类型为Python str,value为一个PCollection.

# transform
- `x.map(fn)`: 对PCollection中的每个元素做一对一映射
- `x.flat_map(fn)`: 对PCollection中的每个元素做一对N映射
- `x.filter(fn)`: 过滤

- `x.count()`
- `x.distinct()`: uniq 操作

- `x.substract(y)`: x - y, 注: 如果有重复元素的话, 需要自己去重
- `x.union(y)`: x + y

- `x.left_join(y)`: 会用None 表示不存在.

	```Python
	>>> x = _pipeline.parallelize([("a", 1), ("b", 4)])
	>>> y = _pipeline.parallelize([("a", 2)])
	>>> transforms.left_join(x, y).get()
	[("a", (1, 2)), ("b", (4, None))]
	```
- `x.full_join(y)`
- `x.right_join(y)`

- `x.flatten()`: 对于给定PTable中的key和value中每一个元素，构造(key, value)对，结果保存在PCollection中

	```
	>>> _p = _pipeline.parallelize({"A": [2, 3], "B": [4, 5]})
	>>> transforms.flatten(_p).get()
	[("A", 2), ("A", 3), ("B", 4), ("B", 5)]
	```

将 `cat txt | awk -F '\t' '{if(NF == 2) print $1}'` 翻译为 bigflow

```Python
def fn(line):
	vec = line.split('\t')
	if len(vec) == 2:
		yield(vec[0])

txt.flat_map(fn)
```
用 yield 组成一个generator

```Python
transforms.pipe(txt, "awk -F '\t' '{if(NF == 2) print $1}'")
```

# lazy variable

