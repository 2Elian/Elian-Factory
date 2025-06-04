# ElianFactory: 在Windows系统上微调大语言模型变得容易
本项目开发了一款可以在Windows系统上进行大模型微调训练的工具，微调推理框架基于transformers库进行开发。

如果您是一名LLM工程师，您可以自行构建您的微调推理代码。
如果您熟悉Docker与Linux虚拟机的安装与操作您可以选择[LlamaFactory](https://www.markdownguide.org)进行LLM微调与推理。但如果您是一名小白，且仅有Windows操作系统，ElianFactory是您的最佳选择。

## ElianFactory的优势

- 🖥️ **基于Windows系统开发**：ElianFactory完全支持原生态Windows系统，无需部署Linux虚拟机即可进行大语言模型的微调和推理。
- 🚀 **直观的操作页面**：ElianFactory是一个无需构建任何代码的工具，您只需进行简单的配置即可进行LLM的微调与推理。
- 🔍 **训练支持**：在ElianFactory-V1.0.0中，我们只支持SFT训练。在后续的版本中，我们会将DPO、PPO、GRPO等功能集成到ElianFactory中，并会考虑更高效的微调推理框架。

## 演示视频

## 快速开始构建项目

### 环境要求

- Windows 10/11
- NVIDIA GPU (支持CUDA)
- C++编译器 (Visual Studio 2019+ 或 MinGW-w64)
- Python 3.10 (用于实际训练)
- Node.js 14+ (用于Vue.js前端开发)

### 安装步骤

1. 克隆仓库：
   ```bash
   git clone https://github.com/yourusername/llm-trainer.git
   cd llm-trainer
   ```

2. 安装Python依赖：(*[Anaconda安装教程](https://www.markdownguide.org)*)
    ```bash
    conda create -n elianfactory python=3.10
    conda activate elianfactory
    pip install torch==2.4.0 torchvision==0.19.0 torchaudio==2.4.0 --index-url https://download.pytorch.org/whl/cu121
    ```
    ```bash
    pip install -r requirements.txt
    ```

3. 安装node和C++：

   *[nodejs安装教程](https://www.markdownguide.org)*

   *[C++安装教程](https://www.markdownguide.org)*

4. 构建项目：
   ```bash
   cd Elian-Factory
   ./build.bat
   ```

5. 启动服务器：
   ```bash
   ./llm_trainer_server.exe
   ```

6. 在浏览器中访问：
   ```
   http://localhost:10171
   ```
   <font color=red>注意：以上步骤只在您初次构建ElianFactory的时候进行操作，如果您已经完成以上步骤，后续使用的时候您只需要点击llm_trainer_server.exe即可。</font>

## 使用教程


## ElianFactory支持的模型


## 开发者信息

- 仓库原作者：@Elian
- 邮箱：lizimo@nuist.edu.cn
欢迎大家一起对本仓库进行共建！

## 后续改进
非常感谢您使用ElianFactory，我们诚挚的邀请您对ElianFactroy做出您使用过程中的真实评价，以便于我们后续的更新改进。以下是我们对后续版本的功能扩展：
- V1.0.1
   
   增添数据生成功能、微调模型集成到Ollama


## 许可证

MIT License