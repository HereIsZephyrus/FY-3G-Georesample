# FY-3G数据集空间重采样算法

## 计算数据集空间直角坐标
### 1. 将地表大地坐标转换为空间直角坐标
数据集中Geolocation组的ellipsoidBinOffset记录了地球椭球表面到脉冲中心的距离，Latitude中第一层记录各测量点地面经度，Longitude第一层记录各测量点地面纬度。记第 $i$ 条扫描带的第 $j$ 个扫描角的数据下标为 $(i,j)$ 。
对任意一点，记大地坐标为 $(B,L,H)$ 则其空间直角坐标系坐标为: 

$$
\begin{aligned}
X &= (N + H) \cos B \cos L \\
Y &= (N + H) \cos B \sin L \\
Z &= (N(1 - e^2) + H) \sin B \\
\end{aligned}
\tag{1}
$$

其中 $N = \frac {a}{(1 - e^2 \sin^2 B)^{\frac 1 2}}$ 为卯酉圈半径， $H$ 为ellipsoidBinOffset记录的偏差值， $e$ 是地球的第一偏心率。

已知数据使用WGS84坐标系，则长半轴 $a=6378137m$ , 短半轴 $b=6356752.3142m$ , 第一偏心率 $e = \frac{\sqrt{a^2 - b^2}}{a}$ , 带入计算。

### 2. 根据局部天顶角将 $18km$ 大地坐标转换为空间直角坐标
由于离地（离地约 $18km$ ）数据没有高程记录，因此需要根据局部天顶角 $Ζ^{(i,j)}$ 数据推算。记实际参考椭球和离地数据空间直角坐标分别为 $(X_g,Y_g,Z_g)$ 和 $(X_a,Y_a,Z_a)$ ，其中 $H_a$ 为未知数，则由式(1)可将 $(X_a,Y_a,Z_a)$ 表示为关于 $H_a$ 的函数，则：

$$
\cos Ζ = \frac{H_a - H_g}{\sqrt{(X_a - X_g)^2 + (Y_a - Y_g)^2 + (Z_a - Z_g)^2}} \tag{2}
$$

化简为二次函数：

$$
\alpha \cdot H_a^2 + \beta \cdot H_a + \gamma = 0 \tag{3}
$$

其中

$$
\begin{cases}
\alpha &= 1 - \frac{1}{\cos^2 Ζ} \\
\beta &= \frac{2H_g}{\cos^2 Ζ} + 2N_a(1-e^2\sin^2 B) -2(\cos B \cos L X_g + \cos B \sin L Y_g + \sin B Z_g)\\
\gamma &= X_g^2 + Y_g^2 + Z_g^2 - \frac{H_g^2}{\cos^2 Ζ} + N_a^2(1 - 2 e^2 \sin^2 B_a + e^4 \sin^2 B_a) -2N_a(\cos B_a \cos L X_g + \cos B_a \sin L Y_g + Z_g(1-e^2) \sin B_a)
\end{cases}
\tag{4}
$$

其中$\alpha < 0$, 因此 $H_a = \frac{-\beta - \sqrt{\Delta}}{2\alpha}$ 。求解二次函数得到离地数据高程，进一步得到离地数据空间直角坐标。

### 3. 根据地表和 $18km$ 格点信息重建所有格点空间直角坐标

Geolocation中height记录的各记录点的海拔高程，从而得到所有格点空间直角坐标系下的坐标。

~~**注意，height字段没有考虑地球曲率！**~~
> ~~经手动验证，数据记录的地形高程(evaluation)与地表的海拔高度(height)有不匹配，原因是天顶角增大后地球曲率影响明显。(卡了我半天)~~
> 不好说，我又看了一下好像只是地表格点标的有可能有偏移

数据集中PRE组的binRealSurface记录了地表数据的格点坐标，binFirstLation记录了离地数据的格点坐标。对于每一格点，不考虑对流层折射情况下，根据地面数据和离地数据写出一次脉冲的表达式:

$$
l : \frac{X - X_g}{X_a - X_g} = \frac{Y - Y_g}{Y_a - Y_g} = \frac{Z - Z_g}{Z_a - Z_g} \tag{5} = \frac{H-H_g}{H_a - H_g}
$$

其中 $H$ 为记录的海拔高度。

### 4. 重建所有格点大地坐标
对任意点 $P$ 空间直角坐标 $(X,Y,Z)$ ，可使用迭代法计算经纬度。
首先计算经度$L$: 

$$
L = 
\begin{cases}
\arctan (\frac{Y}{X}), &(X>0) \\
\arctan (\frac{Y}{X}) + \pi, &(X<0, Y>0) \\
\arctan (\frac{Y}{X}) - \pi, &(X<0, Y<0) \\
\end{cases}
\tag{6}
$$

然后迭代求解纬度 $B$ 。对第 $k$ 次估计 $(k>0)$

$$
\begin{aligned}
B_k &= \arctan \left(\frac{Z + \Delta Z_k}{\sqrt{X^2+Y^2}} \right) \\
H_k &= \sqrt{X^2+Y^2 + (Z+\Delta Z_k)^2}- N_k\\
N_k &=\frac{a}{\sqrt{1-e^2 \sin^2 B_k^2}} \\
\Delta Z_k &= 
\begin{cases}
e^2Z, &k=0 \\
N_{k-1}e^2\sin^2B_{k-1}, &k>0
\end{cases}
\end{aligned}
\tag{7}
$$

但该方法效率较低，而当 $H$ 大于 $10^5m$ 时Bowring算法精度迅速下降无法满足本例的需求，因此在跑通测试后计划使用拉格朗日法进行优化。具体算法查看论文，以下简要说明。

令 $W = \sqrt{X^2 + Y^2}$ ，引入归化纬度 $\mu$ ，则 $P=(W,Z)$ 在椭球面上的投影 $P_0=(a\cos \mu, b \sin \mu)$ ，因此有: 

$$
\begin{cases}
W = a \cos \mu + H \cos B \\
Z = b \sin \mu + H \sin B \\
\sin \mu = \frac{\sqrt{1 - e^2}\sin B}{\sqrt{1 - e^2\sin^2B}}
\end{cases}
\tag{8}
$$

根据几何关系进一步得到: 

$$
\sin \mu = \frac{\sqrt{1 - e^2}Z + ae^2 \sin \mu}{\sqrt{W^2 + (\sqrt{1-e^2}Z + ae^2\sin \mu)^2}}
\tag{10}
$$

记 $s = \sin \mu$ , $w=\frac W a$ , $z = \frac{\sqrt{1-e^2}Z}{a}$ ，移项得到: 

$$
s = \frac{z + e^2s}{\sqrt{w^2 + (z^2 + e^2s)^2}} \tag{11}
$$

因此可以构造函数构造函数: 

$$
f(s) = \frac{z}{\sqrt{w^2+z^2}} = s - \frac{z + e^2s}{\sqrt{w^2 + (z^2 + e^2s)^2}} + \frac{z}{\sqrt{w^2+z^2}} \tag{22}
$$

$s=0$ 时由式(8)可得 $\sin B=0$ ，因此可知 $f(0) = 0$ ,计算后得知当 $(w^2 +z^2)^{\frac 3 2} \neq e^2w^2$ 时 $f'(0) \neq 0$ 。将其泰勒展开，由拉格朗日反演定理有: 

$$
\begin{cases}
f(s) &= \sum_{n=1}^{\infty}\frac{f^{(n)}(0)}{n!}s^n = \sum_{n=1}^{\infty}a_n s^n \\
s &=\sum_{n=1}^{\infty}b_n \left( \frac{z}{\sqrt{w^2+z^2}} \right)^n
\end{cases}
\tag{23}
$$

计算泰勒各项与拉格朗日反演各项，最终得到: 

$$
\begin{aligned}
s = \sin\mu =& t_1 - \frac{3e^4}{2r^2}t_1^3t_4^2 + \frac{e^6}{2r^3}t_1^3t_2^2(4t_3^2 - t_4^2) \\
&+ \frac{e^8}{2r^4}t_1^3t_4^4(5t_1^2 + t_2^2) + \frac{5e^8}{8r^4}t_1^5t_2^2(3t_4^2 - 4t_3^2) \\
&- \frac{10e^{10}}{r^5}t_1^7t_4^4 - \frac{5e^{12}}{8r^6}t_1^5t_4^6(7t_1^2 + 3t_2^2) \\
&+ \frac{3e^{10}}{8r^5}t_1^5t_2^2(8t_1^2t_3^2 - 12t_1^2t_4^2 + t_2^2t_4^2) - \frac{3e^{12}}{8r^6}t_1^5t_2^4(t_4^4 + 25t_3^2t_4^2 - 68t_3^4) \\
&- \frac{3e^{14}}{8r^7}t_1^5t_2^4t_4^2(t_4^4 - 23t_3^2t_4^2 - 92t_3^4) + \frac{3e^{16}}{8r^8}t_1^5t_2^4t_4^4(t_4^4 + 14t_3^2t_4^2 + 21t_3^4) 
\end{aligned}
\tag{24}
$$

其中: 

$$
\begin{cases}
r &= \frac{R}{a} = \frac{\sqrt{X^2 + Y^2 + (1 - e^2)Z^2}}{a} \\
t_1 &= \frac{\sqrt{(1 - e^2)}Z}{R - ae^2(X^2 + Y^2)/R^2} \\
t_2 &= \frac{\sqrt{X^2 + Y^2}}{R - ae^2(X^2 + Y^2)/R^2} \\
t_3 &= \frac{\sqrt{(1 - e^2)}Z}{R} \\
t_4 &= \frac{\sqrt{X^2 + Y^2}}{R}
\end{cases} \tag{25}
$$

在计算得到 $s$ 后，由式(8)和几何关系可得: 

$$
\begin{cases}
B &= \arcsin \left( \frac{s}{\sqrt{1-e^2+e^2s^2}}\right) \\
H &= \sqrt{(W - a\sqrt{1-s^2})^2 + (Z - bs)^2}
\end{cases}
\tag{26}
$$

## 建立三维控制网

### 1. 建立经纬度插值控制网

$200m$ 取一层，共取60层(一般情况下除超强风暴不会超过 $13000m$ ，优化分辨率)。
FY-3G的轨道倾角仅有约为 $50^\circ$ (样例数据集提供的轨道倾角约为 $138.8^\circ$ )，因此单扫描线上不同扫描角经度和纬度均有较大变化。同时卫星从南极扫到北极，纬度圈半径变化极大，需要对不同纬度采取不同的投影以实现比较均匀的插值。而经度圈半径相对固定，本算法根据纬度范围等分，再分别计算数据集在各个纬度带内的经度范围。

其中，为了坐标计算方便，若数据为降轨数据则先将其翻转为升轨数据保证经度单增，对于越过 $180^\circ$ 经线的数据继续累加，在导出结果时再还原，保证插值连续性。

若待插值格网尺寸为 $d$ 米，则经度圈插值步长为: 

$$
B_{gap} = d \cdot \frac{180^\circ}{\pi b} \tag{27}
$$

其中$b$为地球短半轴半径为常数。

遍历数据，得到全体数据经度范围 $[B_{\min}, B_{\max}]$ 和纬度范围 $[L_{\min}, L_{\max}]$ 。设定最大纬度圈宽度为 $P^\circ$ 则切片数量为$N_c=\lceil\frac{B_{\max} - B_{\min}}{P} \rceil$ ，切片的纬度圈宽度 $B_r=\frac{B_{\max} - B_{\min}}{N_c}$ ，各切片纬度范围为 $[B_{\min},B_{\min}+B_r], [B_{\min}+B_r,B_{\min}+2B_r],\ldots, [B_{\min}+(N_c-1)B_r,B_{\min}+N_cB_r]$ 。

再逐个计算各纬度圈的经度范围。由于单扫描线上经纬度变化是单调的，即对于升规卫星数据第 $i$ 条扫描线上 $m$ 个扫描角的经度数据 $B(i,1), B(i,2), \ldots B(i,m)$ 必有: 

$$
\begin{cases}
B_{i-1,j} < B_{i,j} \\
B_{i,j-1} > B_{i,j}
\end{cases}
\tag{28}
$$

从而可以使用二分方法缩小索引范围。即(对于升轨卫星数据)纬度圈切片的最大经度只可能出现在第 $m$ 个扫描角的数据值，最小经度只可能出现在第 $1$ 个扫描角的数据值，从而得到第 $c$ 个切片经度的范围 $[L_{\min}^c,L_{\max}^c]$ 和插值步长: 

$$
G_{gap}^c = d \cdot \frac{180^\circ}{\pi a\cos(B_{\min}+\frac{2c-1}{2}B_r)}
\tag{29}
$$
其中$b$为地球长半轴半径为常数。

### 2. 根据目标垂直分辨率建立三维控制网
由于数据从南极扫到北极，地面高程差太大（超过 $5000m$ ），因此在垂直方向上，从海拔 $H_{\min} = 100m$ ，每 $H_{gap} = 200m$ 取一层，共取 $N_h=60$ 层(一般情况下除超强风暴不会超过 $13000m$ ，优化分辨率)。
三维可视化只关心地表以上的数据。经检查原数据集实际地表的距离单元号binRealSurface在后向散射等干扰时部分点有明显偏差，因此根据evaluation重新确定实际地表的距离单元号，并舍弃低于实际地表海拔高度的数据。此外，数据中还包含不受杂波干扰的距离单元底部单元号binClutterFreeBottom数据，进一步舍去杂波下界以外的数据增强数据质量。根据以上规则整理原始数据得到用于插值的数据集。

## 将数据重采样到控制网

### 建立空间索引

由于数据集点规模极大(理论上一共 $2 \times 3883 \times 59 \times 500 = 229,097,000$ 左右的值很夸张)，待插值的位置也非常多，需要建立空间索引才能进行重采样。使用 [libspatialindex](https://github.com/libspatialindex/libspatialindex) 库的R-star树建立三维空间索引并求KNN，但是由于待插值格网是平行于经纬度的，因此会存在相当多的点距离记录的数据很远，会严重降低在R-star中求KNN的效率。因此本算法在三维的R-star树的基础上再构建一个分层的二维KD树用于快速判断在某一高度层内的经纬度距离。

即建立一个大小为 $N_h+1$ 的KD森林，第 $i$ 棵KD树索引的范围是 $[H_{\min}+(i-1) \cdot H_{gap},H_{\min}+i \cdot H_{gap}]$ ，对于高度为 $h$ 的点，认为其所属于第 $l=\lceil \frac{h-H_{\min}}{H_{gap}} \rceil$ 层，则将其经纬度插入第 $l$ 棵KD树中。在查询时，如果查询点到第 $l-2$ 到第 $l+2$ 层的KD树中最近点距离小于 $0.1^\circ$则认为其有可能有有效值用于插值，再在R-star树中求KNN用于插值

### 反距离加权插值

[libspatialindex](https://github.com/libspatialindex/libspatialindex) 提供的R-star树包含查询带距离的KNN，因此直接插值即可:

$$
w= \sum_{i=1}^n (\frac{w_i}{d_i^2}) / \sum_{i=1}^n (\frac{1}{d_i^2})\tag{30}
$$
