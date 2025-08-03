# 三维R*树使用指南

本文档介绍如何使用基于libspatialindex的三维R*树封装。

## 概述

我们在 `include/index.h` 中封装了libspatialindex的C API，提供了一个易于使用的三维R*树接口。

## 主要特性

- 支持三维点和边界框的插入、删除
- 提供空间查询（相交、包含、最近邻）
- 支持内存和磁盘存储
- 完整的错误处理和资源管理
- 基于成熟的libspatialindex库

## 基本用法

### 1. 创建索引

```c
#include "index.h"

// 创建内存索引
RStar3DIndex* index = CreateRStar3DIndex(RT_Memory, NULL, 100, 0.7);

// 创建磁盘索引  
RStar3DIndex* diskIndex = CreateRStar3DIndex(RT_Disk, "myindex.dat", 100, 0.7);
```

### 2. 插入数据

```c
// 插入点数据
Point3D* point = CreatePoint3D(1.0, 2.0, 3.0, 123, "用户数据", 8);
bool success = RStar3DIndex_InsertPoint(index, point);

// 插入边界框数据
BoundingBox3D* bbox = CreateBoundingBox3D(0.0, 0.0, 0.0, 5.0, 5.0, 5.0);
success = RStar3DIndex_InsertBoundingBox(index, 456, bbox, "边界框数据", 10);
```

### 3. 空间查询

```c
// 相交查询
BoundingBox3D* queryBox = CreateBoundingBox3D(1.0, 1.0, 1.0, 4.0, 4.0, 4.0);
SpatialQueryResult3D* result = RStar3DIndex_IntersectionQuery(index, queryBox);

// 最近邻查询
Point3D* queryPoint = CreatePoint3D(2.5, 2.5, 2.5, -1, NULL, 0);
result = RStar3DIndex_NearestNeighborQuery(index, queryPoint, 5);

// 相交计数
unsigned int count = RStar3DIndex_IntersectionCount(index, queryBox);
```

### 4. 清理资源

```c
// 清理查询结果
DestroySpatialQueryResult3D(result);

// 清理几何对象
DestroyPoint3D(point);
DestroyBoundingBox3D(bbox);

// 清理索引
DestroyRStar3DIndex(index);
```

## 单元测试

我们提供了完整的Unity单元测试，位于 `tests/unit_RTree.c`。

测试包括：
- 基础功能测试
- 边界框操作测试  
- 最近邻查询测试
- 错误处理测试

运行测试：
```bash
./test_runner
```

## 编译说明

确保链接libspatialindex库：
```bash
gcc -o myprogram myprogram.c src/rstar3d_index.c -lspatialindex_c -lspatialindex -lm
```

## 性能建议

1. **节点容量**: 通常50-100是好的起点
2. **填充因子**: 推荐0.7
3. **存储类型**: 大数据集使用磁盘存储
4. **内存管理**: 及时释放查询结果和临时对象 