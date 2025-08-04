# FY-3G空间重采样工具

## 项目简介

FY-3G空间重采样工具是一个用于处理FY-3G卫星降水雷达数据的高性能C语言应用程序，支持OpenMP并行计算，集成R*树、KD树空间索引优化大数据处理性能。该工具能够将原始HDF5格式的三维降水数据转换为大地坐标系统，并根据采样间隔对不同纬度带切片重采样。

![](https://cdn.jsdelivr.net/gh/HereIsZephyrus/zephyrus.img/images/blog/FY-3Gdemo.png)

算法详见[docs/algorithm.md](docs/algorithm.md)
项目说明文档详见[docs/project.md](docs/project.md)

## 系统要求

### 基本要求
- **操作系统**：Linux
- **编译器**：GCC 7.0+ 或 Clang 6.0+（需要支持C11标准）
- **CMake**：3.21+
- **内存**：运行时需保证10GB可用内存
- **存储空间**：建议10GB以上可用空间

### 依赖库
- **HDF5**：5.x版本
- **OpenMP**：并行计算支持
- **libspatialindex**：空间索引库

## 安装方法

### 1. 克隆项目
```bash
git clone --recursive <项目仓库地址>
cd FY-3G-Georesample
```

### 2. 安装系统依赖
#### Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake gcc g++ libomp-dev zlib1g-dev
```

#### CentOS/RHEL:
```bash
sudo yum groupinstall "Development Tools"
sudo yum install cmake gcc gcc-c++ openmp-devel zlib-devel
```

### 3. 构建项目
```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

### 4. 运行测试（可选）
```bash
./FY3G_Resampling_test
```

## 使用方法

### 基本用法
```bash
./FY3G_Resampling_exe <配置文件路径>
```

### 示例
```bash
./FY3G_Resampling_exe /path/to/resample.config
```

## 配置文件说明

配置文件采用简单的键值对格式，支持以下参数：

### 文件路径配置
- **INPUT_FILE_NAME**：输入的FY-3G HDF5数据文件路径
- **OUTPUT_FILE_NAME**：输出文件路径

### 空间处理参数
- **MAX_LONGITUDE_WIDTH**：最大经度宽度（度）
  - 默认值：5.0
  - 作用：限制切片处理区域的经度范围

- **GRID_SIZE**：网格大小（米）
  - 默认值：5000
  - 作用：定义输出网格的空间分辨率

### 高度相关参数
- **MINIMAL_HEIGHT**：最小处理高度（米）
  - 默认值：100
  - 作用：设置数据处理的最低高度阈值

- **HEIGHT_GAP**：高度间隔（米）
  - 默认值：200
  - 作用：定义垂直方向的采样间隔

- **HEIGHT_COUNT**：高度层数
  - 默认值：60
  - 作用：指定处理的垂直层数

### 插值算法参数
- **K_NEIGHBOR**：K近邻数量
  - 默认值：5
  - 作用：空间插值时使用的邻近点数量

- **MAX_DISTANCE_TOLERANCE**：最大距离容差（度）
  - 默认值：0.08
  - 作用：可能执行插值的空间距离阈值

- **MAX_NEIGHBOR_DISTANCE**：最大邻居距离（米）
  - 默认值：10000
  - 作用：搜索邻近点的最大距离范围

- **MIN_NEIGHBOR_DISTANCE**：最小邻居距离（米）
  - 默认值：100
  - 作用：避免过近点的最小距离阈值

### 性能优化参数
- **KDTREE_CAPACITY**：KD树容量
  - 默认值：100000
  - 作用：优化空间索引的内存使用

- **BATCH_SIZE**：批处理大小（已弃用）
  - 默认值：500
  - 作用：控制批处理的数据量

### 配置文件示例
```ini
INPUT_FILE_NAME=/path/to/FY3G_PMR_data.HDF
OUTPUT_FILE_NAME=/path/to/output_data.HDF
MAX_LONGITUDE_WIDTH=5
MINIMAL_HEIGHT=100
HEIGHT_GAP=200
HEIGHT_COUNT=60
K_NEIGHBOR=5
KDTREE_CAPACITY=100000
BATCH_SIZE=500
GRID_SIZE=5000
MAX_DISTANCE_TOLERANCE=0.1
MAX_NEIGHBOR_DISTANCE=100000
MIN_NEIGHBOR_DISTANCE=100
```

## 输入输出格式

### 输入格式
- **文件类型**：HDF5格式
- **数据内容**：FY-3G降水雷达原始数据
- **数据结构**：包含地理位置、高度、降水强度等信息

### 输出格式
- **文件类型**：HDF5格式
- **数据内容**：重采样后的降水数据
- **坐标系统**：大地坐标系（WGS84）

## 许可证

本项目采用MIT许可证，详见[LICENSE](LICENSE)文件。
