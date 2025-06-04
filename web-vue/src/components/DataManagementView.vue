<template>
  <div class="data-management-view">
    <div class="row mt-3">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white">
            <i class="bi bi-database me-2"></i>数据管理
          </div>
          <div class="card-body">
            <div class="alert alert-info">
              <i class="bi bi-info-circle me-2"></i>
              此功能正在开发中，敬请期待！
            </div>
            
            <!-- 未来会实现的功能预览 -->
            <div class="row mt-4">
              <div class="col-md-6 mb-4">
                <div class="card h-100 hover-scale">
                  <div class="card-body">
                    <h5 class="card-title">
                      <i class="bi bi-file-earmark-text me-2 text-primary"></i>
                      数据集预处理
                    </h5>
                    <p class="card-text">上传并处理训练数据，支持多种格式转换和数据清洗功能。</p>
                  </div>
                </div>
              </div>
              <div class="col-md-6 mb-4">
                <div class="card h-100 hover-scale">
                  <div class="card-body">
                    <h5 class="card-title">
                      <i class="bi bi-bar-chart me-2 text-success"></i>
                      数据集可视化
                    </h5>
                    <p class="card-text">分析数据分布，检查数据质量，提供数据集统计信息。</p>
                  </div>
                </div>
              </div>
              <div class="col-md-6 mb-4">
                <div class="card h-100 hover-scale">
                  <div class="card-body">
                    <h5 class="card-title">
                      <i class="bi bi-files me-2 text-info"></i>
                      数据集管理
                    </h5>
                    <p class="card-text">管理和组织多个数据集，支持数据集的合并、分割和筛选。</p>
                  </div>
                </div>
              </div>
              <div class="col-md-6 mb-4">
                <div class="card h-100 hover-scale">
                  <div class="card-body">
                    <h5 class="card-title">
                      <i class="bi bi-magic me-2 text-warning"></i>
                      数据增强
                    </h5>
                    <p class="card-text">使用先进的增强技术扩充训练数据，提高模型泛化能力。</p>
                  </div>
                </div>
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script>
export default {
  name: 'DataManagementView'
}
</script>

<style scoped>
.card-title {
  font-size: 1.2rem;
  font-weight: 600;
}

.hover-scale {
  transition: transform 0.2s ease;
}

.hover-scale:hover {
  transform: translateY(-5px);
  box-shadow: 0 10px 20px rgba(0, 0, 0, 0.1);
}
</style>
 