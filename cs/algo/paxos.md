Paxos 分布式一致性协议
# intro
[paxos入门介绍](http://www.tudou.com/programs/view/e8zM8dAL6hM)

在Lamport 的The Part-Time Parliament 论文的[解说中](http://research.microsoft.com/en-us/um/people/lamport/pubs/pubs.html#lamport-paxos),
"at the heart of the algorithm is a three-phase consensus protocol", 而这个视频的讲述中, 说是一个两阶段的.

Byzantine failures

拜占庭将军问题是一个协议问题,拜占庭帝国军队的将军们必须全体一致的决定是否攻击某一支敌军. 问题是这些将军在地理上是分隔开来的,并且将军中存在叛徒.

叛徒可以任意行动以达到以下目标:
欺骗某些将军采取进攻行动,
促成一个不是所有将军都同意的决定,如当将军们不希望进攻时促成进攻行动,
或者迷惑某些将军,使他们无法做出决定.
如果叛徒达到了这些目的之一,则任何攻击行动的结果都是注定要失败的,只有完全达成一致的努力才能获得胜利.

拜占庭假设是对现实世界的模型化,由于硬件错误,网络拥塞或断开以及遭到恶意攻击,计算机和网络可能出现不可预料的行为.
拜占庭容错协议必须处理这些失效,并且这些协议还要满足所要解决的问题要求的规范.这些算法通常以其弹性t作为特征,t表示算法可以应付的错误进程数.

很多经典算法问题只有在t<n/3时才有解,如拜占庭将军问题,其中n是系统中进程的总数.

paxos 究竟在解决什么问题?

- paxos 用来**确定一个不可变变量的取值**
	- 取值可以是任意二进制数据
	- 一旦确定将不在更改, 并且可以被获取到(不可变性, 可读取性)

paxos 如何在分布式存储系统中应用?

- 数据本身可变, 采用多副本进行存储
- 多个副本的更新操作序列[Op1, Op2, ..., Opn] 是相同的, 不变的.
- 用paxos一次来确定不可变变量Opi的取值(即第i个操作是什么)
- 每确定完Opi之后, 让各个数据副本来执行Opi, 以此类推.
- Google 的Chubby, Megastore 和 Spanner 都采用了Paxos 来对数据副本的更新序列达成一致.

paxos 算法的核心思想是什么?

- 第一阶段在做什么?
- 第二阶段在做什么?

Paxos 希望解决的一致性问题

1. 设计一个系统, 来存储名称为var 的变量
	- 系统内部由多个Acceptor 组成, 负责存储和管理var变量
	- 外部有多个proposer 机器任意并发调用API, 向系统提交不同的var取值
	- var 的取值可以是任意的二进制数据
	- 系统对外的API库接口为: proposer(var, V) => <ok, f> or <error>
		- 若var 之前为空, 则var会被成功的更新, f 会等于V
		- 若var 之前已经有值了, 则返回的f 为之前已经确定的值
1. 系统需要保证var的取值满足一致性
	- 如果var 的取值还没有确定, 则var 的取值为null
	- 一旦var 的取值被确定, 则不可被更改. 并且可以一直获取到这个值
1. 系统需要满足容错特性
	- 可以容忍任意proposer机器出现故障
	- 可以容忍半数以下的acceptor故障
1. 为了讲解简单, 暂不考虑
	- 网络分化
	- acceptor故障丢失var的信息

确定一个不可变变量的难点

- 管理多个proposer 的并发执行
- 保证var变量的不可变性
- 容忍任意proposer机器的故障
- 容忍半数以下acceptor机器故障

# 分布式系统的一致性
Paxos一致性协议是在特定的环境下才需要的,这个特定的环境称为异步通信环境.
而恰恰,几乎所有的分布式环境都是异步通信环境,在计算机领域面对的问题,非常需要Paxos来解决.
异步通信环境指的是消息在网络传输过程中,可能发生丢失,延迟,乱序现象

## 方案1: 基于互斥锁的互斥访问权
### 原理
- 先考虑系统由单个acceptor组成, 通过类似互斥锁的机制, 来管理并发的proposer 运行
- proposer 首先想acceptor申请acceptor的互斥访问权, 再得到互斥访问权之后, 才能请求acceptor接收自己的取值
- acceptor给proposer发放互斥访问权, 谁申请到互斥访问权, 就接收谁提交的取值.
- 让proposer按照互斥访问权的顺序依次访问acceptor
- 一旦acceptor接收了某个proposer的取值, 则认为var取值被确定, 其他proposer不再更改.

### 实现
基于互斥访问权的acceptor的实现

- Acceptor保存变量var 和一个互斥锁lock
- Acceptor::prepare(): 加互斥锁, 给予var的互斥访问权, 并返回var 当前的取值f
- Acceptor::release(): 释放互斥锁, 收回var 的互斥访问权
- Acceptor::accept(var, V): 如果已经加锁, 并且bar没有取值, 则设置var 为V. 并且释放锁.


propose(var, V)的两阶段实现

- 第一截断: 通过Acceptor::prepare() 获取互斥访问权和当前var的取值
	- 如果不能, 说明锁被别人占用, 返回<error>
- 第二阶段: 根据当前var 的取值, 选择执行
	- 如果f 为null, 则通过Acceptor::accept(var, V) 提交数据V
	- 如果f 不为null, 则通过Acceptor::release() 释放访问权, 返回<ok, f>

### 思考
proposer 在释放互斥访问权之前发生故障, 会导致系统陷入死锁.
因此方案1不能容忍任意proposer机器故障.

## 方案2: 抢占式访问权
### 原理
- 引入抢占式访问权
	- acceptor 可以让某个proposer 获取到的访问权失效, 不在接收它的访问.
	- 之后, 可以将访问权发放给其他proposer, 让其他proposer访问acceptor.

- proposer 向acceptor 申请访问权时**指定编号epoch(越大的epoch越新)**, 获取到访问权之后, 才能向acceptor 提交取值.

- acceptor 采用**喜新厌旧**的原则
	- 一旦收到更大的新epoch 的申请, 马上让旧的 epoch 的访问权失效, 不在接受他们提交的取值
	- 然后给新的epoch 发放访问权, 只接收新epoch 提交的取值.

- 新epoch 可以抢占旧epoch, 让旧的epoch 的访问权失效. 旧的epoch 的 proposer 将无法运行, 新的epoch 的proposer 将开始运行.
	- **当一直有新的epoch 让旧的epoch 失效的话, 就会无休无止, 成为活锁**

- 为了保持一致性, 不同epoch 的proposer 之间采用**后者认同前者**的原则
	- 在肯定旧的epoch 无法生成确定性取值时, 新的epoch 会提交自己的value, 不会冲突
	- 一旦旧的epoch 形成确定性取值, 新的epoch可以获取到此取值, 并且会认同此取值, 不会破坏.

### 实现
基于抢占式访问权的acceptor 的实现

- acceptor 保存的状态
	- 当前var 的取值<accepted_epoch, accepted_value>, accepted_value 是在 accepted_epoch 时提交的值
	- 最新发放访问权的epoch(latest_prepared_epoch)

- Acceptor::prepare(epoch):
	- 只接收比 latest_prepared_epoch 更大的epoch, 并给予访问权
	- 记录latest_prepared_epoch = epoch: 返回当前var 的取值

- Acceptor::accept(var, prepared_epoch, V):
	- 验证 latest_prepared_epoch == prepared_epoch
	- 并设置var 的取值<accepted_epoch, accepted_value> = <prepared_epoch, v>


Propose(var, V)的两阶段实现

- 第一阶段: 获取epoch 轮次的访问权和当前var 的取值
	- 简单获取当前的时间戳为epoch, 通过acceptor::prepare(epoch) 获取epoch 轮次的访问权和当前var 的取值
	- 如果不能继续, 返回 <error>
- 第二阶段: 采用**后者认同前者**的原则选定取值, 进行提交
	- 如果var 的取值为空, 则肯定当前没有确定性取值, 则通过Acceptor::accept(var, epoch, V)提交数据V.
		- 成功后返回 <ok, V>
		- 失败返回<error>, 说明被新epoch 抢占或者acceptor 故障
	- 如果var 取值存在, 则此取值肯定是确定性取值, 此时认同它不在更改, 直接返回<ok, accepted_value>.

### 思考
- **仍需引入多个acceptor**, 单机模块acceptor的故障导致整个系统宕机, 无法提供服务.
- 活锁

## 方案3: Paxos
Acceptor

- paxos 在方案2的基础上引入多个acceptor
	- acceptor 的实现保持不变, 仍采用**喜新厌旧**的原则运行.
- 由于引入了多个acceptor, 采用**少数服从多数**的原则
- 一旦**某个epoch 的取值f 被半数以上acceptor接受**, 则认为此var取值被确定为f, 不再更改.

Proposer:

- Propose(var, V)第一阶段: 选定epoch, 获取epoch 访问权和对应的var 取值
	- 获取半数以上acceptor 的访问权和对应的一组var 取值
- Propose(var, V)第二阶段: 采用"后者认同前者"的原则执行
	- 如果获取的var 取值都为空, 则旧的epoch 无法形成确定性取值, 此时努力使<epoch, V>成为确定性取值.
		- 向epoch 对应的对应的所有acceptor提交取值<epoch, V>
		- 如果收到半数以上(应该是指总的acceptor)成功, 则返回<ok, V>
		- 否则, 则返回<error> (被新的epoch 抢占或者acceptor 故障)
	- 如果var 的取值存在, 认同**最大accpeted_epoch 对应的取值f**, 努力使<epoch, f> 成为确定性取值.
		- 如果f 出现半数以上, 则说明f 已经是确定性取值, 直接返回<ok, f>
		- 否则, 向epoch 对应的所有acceptor 提交取值<epoch, f>

例如: 
一共有100个acceptor, 在初始状态下, var 都为null.
然后, 某个proposer 要提交f1, 在获取了半数以上的acceptor(51个), 但是在成功提交到49个的时候(这时51个中剩下的两个的值仍然是null),
有新的proposer 到来, 将第一个proposer 还未提交生效的acceptor 的访问权抢占了过来, 连并剩下的49个组成了51个, 超过了半数. 且这个proposer 都提交成功了. 也就是现在的状态是有51个f2, 49个f1, f2 的epoch 大于f1 的epoch.
这时, 又有一个新的proposer 来了, 获取了f1 的49个 加上f2 的2个, 共51 个acceptor. 这个proposer 发现虽然有49个f1, 但是没有达到多数, 所以需要认同最大的accpeted_epoch 对应的取值f, 这里就是2个f2 的epoch, 然后这个proposer 会提交 <new_epoch, f2>
可是, 然后呢? 按照前面的规则, 因为这些acceptor 都有值, 好像不能提交成功啊???

### 思考
新轮次的抢占会让旧轮次停止运行, 如果每一轮次在第二阶段执行成功之前都被新一轮抢占, 则导致**活锁**, 怎么解决呢?

# 生产环境
[微信自研生产级paxos类库PhxPaxos实现原理介绍](http://toutiao.com/a6298947203093332225/)

[github](https://github.com/tencent-wechat/phxpaxos)

## 确定多个值
对我们来说,确定一个值,并且当一个值确定后是永远不能被修改的,很明显这个应用价值是很低的.
虽然我都甚至还不知道确定一个值能用来干嘛,但如果我们能有办法能确定很多个值,那肯定会比一个值有用得多.我们先来看下怎么去确定多个值.
上文提到一个三个Acceptor和Proposer各自遵守paxos协议,协同工作最终完成一个值的确定.
这里先定义一个概念,
Proposer,各个Acceptor,所服务的Data共同构成了一个大的集合,这个集合所运行的paxos算法最终目标是确定一个值,我们这里称这个集合为一个paxos instance,即一个paxos实例.
**一个实例可以确定一个值**,那么多个实例自然可以确定多个值,很简单的模型就可以构建出来,只要我们同时运行着多个实例,那么我们就能完成确定多个值的目标.
这里强调一点,每个实例必须是完全独立,互不干涉的.意思就是说Acceptor不能去修改其他实例的Data数据,Proposer同样也不能跨越实例去与其他实例的Acceptor交互.

然而比较遗憾的一点,确定多个值,仍然对我们没有太大的帮助,因为里面最可恨的一点是,当一个值被确定后,就永远无法被修改了,这是我们不能接受的.
大部分的存储服务可能都需要有一个修改的功能.

## 有序的确定多个值
我们需要转换一下切入点,也许我们需要paxos确定的值,并不一定是我们真正看到的数据.
我们观察大部分存储系统,如LevelDB,都是以AppendLog的形式,确定一个操作系列,而后需要恢复存储的时候都可以通过这个操作系列来恢复,
而这个操作系列,正是确定之后就永远不会被修改的.
到这已经很豁然开朗了,只要我们通过paxos完成一个多机一致的有序的操作系列,那么通过这个操作系列的演进,可实现的东西就很有想象空间了,存储服务必然不是问题.

如何利用paxos有序的确定多个值?上文我们知道可以通过运行多个实例来完成确定多个值,但为了达到顺序的效果,需要加强一下约束.

1. 首先给实例一个编号,定义为i,i从0开始,只增不减,由本机器生成,不依赖网络.
1. 其次,我们保证一台机器任一时刻只能有一个实例在工作,这时候Proposer往该机器的写请求都会被当前工作的实例受理.
1. 最后,当编号为i的实例获知已经确定好一个值之后,这个实例将会被销毁,进而产生一个编号为i+1的实例.

基于这三个约束,每台机器的多个实例都是一个连续递增编号的有序系列,而基于paxos的保证,同一个编号的实例,确定的值都是一致的,那么三台机都获得了一个有序的多个值.

下面结合一个图示来详细说明一下这个运作过程以及存在什么异常情况以及异常情况下的处理方式.
![三个约束](http://p3.pstatp.com/large/9400001789ecea80b40)

图中A,B,C代表三个机器,红色代表已经被销毁的实例,根据上文约束,最大的实例就是当前正在工作的实例.
A机器当前工作的实例编号是6,B机是5,而C机是3.
为何会出现这种工作实例不一样的情况?
首先解释一下C机的情况,由于paxos只要求多数派存活即可完成一个值的确定,所以假设C出现当机或者消息丢失延迟等,都会使得自己不知道3-5编号的实例已经被确定好值了.
而B机比A机落后一个实例,是因为B机刚刚参与完成实例5的值的确定,但是他并不知道这个值之前已经被确定了.
上面的情况与其说是异常情况,也可以说是正常的情况,因为在分布式环境,发生这种事情是很正常的.

下面分析一下基于图示状态的对于C机的写入是如何工作的.
C机实例3处理一个新的写入,根据paxos协议的保证,由于实例3已经确定好一个值了,所以无论写入什么值,都不会改变原来的值,
所以这时候C机实例3发起一轮paxos算法的时候就可以获知实例3真正确定的值,从而跳到实例4.但在工程实现上这个事情可以更为简化,上文提到,各个实例是独立,互不干涉的,
也就是A机的实例6,B机的实例5都不会去理会C机实例3发出的消息,那么C机实例3这个写入是无法得到多数派响应的,自然无法写入成功.
再分析一下A机的写入,同样实例6无法获得多数派的响应,同样无法写入成功.
同样假如B机实例5有写入,也是写入失败的结果.
那如何使得能继续写入,实例编号能继续增长呢?这里引出下一个章节.

## 实例的对齐(Learn)
上文说到每个实例里面都有一个Acceptor的角色,这里再增加一个角色称之为Learner,顾名思义就是找别人学习,
她会去询问别的机器的相同编号的实例,如果这个实例已经被销毁了,那说明值已经确定好了,直接把这个值拉回来写到当前实例里面,然后编号增长跳到下一个实例再继续询问,
如此反复,直到当前实例编号增长到与其他机器一致.
由于约束里面保证仅当一个实例获知到一个确定的值之后,才能开始新的实例(编号大于之前的),那么换句话说,只要编号比当前工作实例小的实例(已销毁的),他的值都是已经确定好的.
所以这些值并不需要再通过paxos来确定了,而是直接由Learner直接学习得到即可.

![learner 角色](http://p3.pstatp.com/large/93c000185311d7d8cba)
如上图,B机的实例5是直接由Learner从A机学到的,而C机的实例3-5都是从B机学到的,这样大家就全部走到了实例6,这时候实例6接受的写请求就能继续工作下去.

## 如何应用paxos
状态机
一个有序的确定的值,也就是日志,可以通过定义日志的语义进行重放的操作,那么这个日志是怎么跟paxos结合起来的呢?我们利用paxos确定有序的多个值这个特点,再加上这里引入的一个状态机的概念,结合起来实现一个真正有工程意义的系统.
状态机这个名词大家都不陌生,一个状态机必然涉及到一个状态转移,而paxos的每个实例,就是状态转移的输入,由于每台机器的实例编号都是连续有序增长的,而每个实例确定的值是一样的,那么可以保证的是,各台机器的状态机输入是完全一致的.根据状态机的理论,只要初始状态一致,输入一致,那么引出的最终状态也是一致的.而这个状态,是有无限的想象空间,你可以用来实现非常多的东西.
下图例子告诉大家Proposer,Acceptor,Learner,State machine是如何协同工作的.
![多个角色协同工作](http://p3.pstatp.com/large/94300017fa30da672f8)
一个请求发给Proposer,Proposer与相同实例编号为x的Acceptor协同工作,共同完成一值的确定,之后将这个值作为状态机的输入,产生状态转移,最终返回状态转移结果给发起请求者.

## 工程化
### 我们需要严格的落盘
当操作系统告诉我写盘成功,那么无论任何情况都不会丢失.这个我们一般使用fsync来解决问题,也就是每次进行写盘都要附加一个fsync进行保证.
Fsync是一个非常重的操作,也因为这个,paxos最大的瓶颈也是在写盘上,在工程上,我们需要尽量通过各种手段,去减少paxos算法所需要的写盘次数.
万一磁盘fsync之后,仍然丢失或者数据错乱怎么办?这个称之为拜占庭问题,工程上需要一系列的措施检测出这些拜占庭错误,然后选择性的进行数据回滚或者直接丢弃.
