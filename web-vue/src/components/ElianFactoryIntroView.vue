<template>
  <div class="container-fluid">
    <div class="row">
      <div class="col-12">
        <div class="card">
          <div class="card-header bg-primary text-white">
            <h4>ElianFactory 功能介绍</h4>
          </div>
          <div class="card-body">
            <div class="row">
              <div class="col-md-6 mb-4">
                <h5 class="border-bottom pb-2">什么是ElianFactory?</h5>
                <p>ElianFactory是一个专为Windows平台设计的大型语言模型（LLM）训练和推理平台，旨在为AI研发人员、学术研究者和企业提供便捷的LLM训练环境。无需复杂配置，即可在Windows系统上轻松开展模型训练工作。</p>
                
                <h5 class="border-bottom pb-2 mt-4">核心优势</h5>
                <ul class="feature-list">
                  <li><span class="feature-icon"><i class="bi bi-lightning-charge-fill"></i></span> <strong>简单易用</strong> - 图形化界面，无需编写复杂命令</li>
                  <li><span class="feature-icon"><i class="bi bi-speedometer2"></i></span> <strong>训练方式</strong> - SFT、DPO、PPO、GRPO</li>
                  <li><span class="feature-icon"><i class="bi bi-cpu"></i></span> <strong>多GPU支持</strong> - 支持DDP和DeepSpeed多卡并行训练，提升训练效率</li>
                  <li><span class="feature-icon"><i class="bi bi-box-seam"></i></span> <strong>预配置环境</strong> - 预装常用库和依赖，开箱即用</li>
                  <li><span class="feature-icon"><i class="bi bi-graph-up"></i></span> <strong>实时监控</strong> - 训练过程可视化，实时查看模型性能</li>
                </ul>
              </div>
              
              <div class="col-md-6">
                <h5 class="border-bottom pb-2">适用场景</h5>
                <div class="scenario-card">
                  <h6><i class="bi bi-mortarboard"></i> 学术研究</h6>
                  <p>为研究人员提供简单易用的环境，快速验证研究想法，支持各类LLM架构实验。</p>
                </div>
                
                <div class="scenario-card">
                  <h6><i class="bi bi-building"></i> 企业应用</h6>
                  <p>帮助企业基于通用语言模型进行领域微调，构建特定行业的AI解决方案。</p>
                </div>
                
                <div class="scenario-card">
                  <h6><i class="bi bi-person-workspace"></i> 个人开发者</h6>
                  <p>降低AI开发门槛，使个人开发者也能够训练和部署自己的语言模型。</p>
                </div>
                
                <h5 class="border-bottom pb-2 mt-4">技术规格</h5>
                <table class="table table-sm specs-table">
                  <tbody>
                    <tr>
                      <td><strong>支持模型</strong></td>
                      <td>DeepSeek、Qwen、Llama等主流大模型</td>
                    </tr>
                    <tr>
                      <td><strong>训练方式</strong></td>
                      <td>LoRA、QLoRA等参数高效训练方法</td>
                    </tr>
                  </tbody>
                </table>
              </div>
            </div>
            
            <div class="text-center mt-4">
              <h5 class="border-bottom pb-2">点击左侧菜单中的"模型训练"或"模型推理"选项，开始您的LLM训练之旅！</h5>
            </div>
          </div>
          <div class="card-header bg-primary text-white">
            <h4>ElianFactory 版本介绍</h4>
          </div>
          <div class="card-body">
            <div class="row">
              <div class="col-md-6 mb-4">
                <h5 class="border-bottom pb-2">ElianFactory-V1.0.0</h5>
                <p>本项目开发了一款可以在Windows系统上进行大模型微调训练的工具，微调推理框架基于transformers库进行开发。</p>
                
                <h5 class="border-bottom pb-2 mt-4">V1.0.0核心功能</h5>
                <ul class="feature-list">
                  <li><span class="feature-icon"><i class="bi bi-lightning-charge-fill"></i></span> <strong>SFT训练</strong> - 一键式训练您的LLM</li>
                  <li><span class="feature-icon"><i class="bi bi-speedometer2"></i></span> <strong>训练方式</strong> - LORA、QLORA</li>
                  <li><span class="feature-icon"><i class="bi bi-cpu"></i></span> <strong>多GPU支持</strong> - 支持DDP多卡并行训练，提升训练效率</li>
                  <li><span class="feature-icon"><i class="bi bi-graph-up"></i></span> <strong>实时监控</strong> - 训练过程可视化，实时查看模型性能</li>
                </ul>
              </div>
              
              <div class="col-md-6">
                <h5 class="border-bottom pb-2">开发者信息</h5>
                <div class="scenario-card">
                  <h6><i class="bi bi-person-workspace"></i> Elian</h6>
                  <p>Elian是Elian-Factory的第一发起人，您可以通过lizimo@nuist.edu.cn联系到ta。</p>
                </div>
                
                <h5 class="border-bottom pb-2 mt-4">V1.0.1计划增加功能</h5>
                <table class="table table-sm specs-table">
                  <tbody>
                    <tr>
                      <td><strong>数据生成</strong></td>
                      <td>调用API或者ollama进行数据生成的功能</td>
                    </tr>
                    <tr>
                      <td><strong>集成Ollama</strong></td>
                      <td>将训练好的模型集成到Ollama中进行使用</td>
                    </tr>
                  </tbody>
                </table>
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
  name: 'ElianFactoryIntroView'
}
</script>

<style scoped>
.feature-list {
  list-style-type: none;
  padding-left: 0;
}

.feature-list li {
  margin-bottom: 12px;
}

.feature-icon {
  display: inline-block;
  width: 28px;
  height: 28px;
  line-height: 28px;
  text-align: center;
  border-radius: 50%;
  margin-right: 8px;
  background-color: var(--primary-color);
  color: white;
}

.scenario-card {
  background-color: var(--background-alt);
  padding: 15px;
  border-radius: var(--border-radius);
  margin-bottom: 15px;
}

.scenario-card h6 {
  color: var(--primary-color);
  margin-bottom: 10px;
}

.scenario-card i {
  margin-right: 8px;
}

.specs-table td {
  padding: 8px 12px;
}
</style> 