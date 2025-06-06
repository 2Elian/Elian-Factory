# ElianFactory: 在Windows系统上微调大语言模型变得容易
\[ 中文 | [English](README_en.md) \]

本项目开发了一款可以在Windows系统上进行大模型微调训练的工具，微调推理框架基于transformers库进行开发。

如果您是一名LLM工程师，您可以自行构建您的微调推理代码。
如果您熟悉Docker与Linux虚拟机的安装与操作您可以选择[LlamaFactory](https://www.markdownguide.org)进行LLM微调与推理。但如果您是一名小白，且仅有Windows操作系统，[ElianFactory](https://github.com/2Elian/Elian-Factory)是您的最佳选择。


## ElianFactory的优势

- 🖥️ **基于Windows系统开发**：ElianFactory完全支持原生态Windows系统，无需部署Linux虚拟机即可进行大语言模型的微调和推理。
- 🚀 **直观的操作页面**：ElianFactory是一个无需构建任何代码的工具，您只需进行简单的配置即可进行LLM的微调与推理。
- 🔍 **训练支持**：在ElianFactory-V1.0.0中，我们只支持SFT训练。在后续的版本中，我们会将DPO、PPO、GRPO等功能集成到ElianFactory中，并会考虑更高效的微调推理框架。

## 演示视频

![演示地址](/img/video.gif)

## 快速开始构建项目

### 安装步骤

1. 克隆仓库：
   ```bash
   git clone https://github.com/2Elian/Elian-Factory.git
   cd Elian-Factory
   ```

2. 安装Python依赖：
    ```bash
    conda create -n elianfactory python=3.10
    conda activate elianfactory
    pip install torch==2.4.0 torchvision==0.19.0 torchaudio==2.4.0 --index-url https://download.pytorch.org/whl/cu121
    ```
    ```bash
    pip install -r requirements.txt
    ```

3. 安装node和g++(node可以省略 g++是必须的)：

   *[node安装教程](node_down.md)*

   *[g++安装教程](g_down.md)*

4. 构建项目：
   ```bash
   cd Elian-Factory
   ./build.bat
   ```

   如果运行上述bat脚本出现报错，请按照下面的方法进行操作：
   ```bash
   cd Elian-Factory
   g++ llm_trainer_server.cpp -o llm_trainer_server.exe -std=c++17 -lws2_32
   ```

5. 启动服务器：
   ```bash
   # 最好是右键点击以管理员身份运行
   ./llm_trainer_server.exe
   ```

6. 在浏览器中访问：
   ```
   http://localhost:10171
   ```
   <font color=red>注意：以上步骤只在您初次构建ElianFactory的时候进行操作，如果您已经完成以上步骤，后续使用的时候您只需要点击llm_trainer_server.exe即可。</font>

## 使用教程

### 模型下载
   您可以在huggingface或modelscope上下载您需要微调的大模型，本项目以deepseek-r1:1.5b为例，进行讲解：
   1. 启动cmd
      ```bash
      cd Elian-Factory
      cd llm
      conda activate elianfactory
      mkdir ./ckpt/deepseek1.5
      ```
   2. 下载模型
      ```bash
      # huggingface 命令
      huggingface-cli download deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B --local-dir ./ckpt/deepseek1.5

      # modelscope命令
      modelscope-cli download deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B --local_dir ./ckpt/deepseek1.5
      ```
   3. 在ElianFactory中选择模型
   
      将前端页面的模型路径更换为：/ckpt/deepseek1.5

### 更换数据集

   本项目的数据集均为格式如下：
   ```bash
   {"conversation": [{"human": "这里是问题", "assistant": "这里是答案"}]}

   {"conversation": [{"human": "机器学习是什么？", "assistant": "机器学习是人工智能的一个分支，它使计算机系统能够通过经验自动改进。具体来说，机器学习算法通过使用计算方法从数据中学习信息，而无需明确编程。机器学习模型通常分为监督学习、无监督学习和强化学习等类型。"}]}
   ```

   您可以根据以上格式构建您自己的数据集(注意：数据集后缀名应为jsonl)，并将构建好的数据集存入到Elian-Factory/llm/data目录下。


### 调整超参数

   Elian-Factory可供您选择的参数如下：
   - 模型路径：存放需要微调LLM的路径
   - 输出目录： 您需要将微调模型的权重放置到哪个目录下
   - 训练数据文件： 您可以在页面中进行选择您所需要训练的数据集
   - 最大序列长度： 模型期待输出的最大token数
   - 训练轮数： 模型需要迭代多少轮
   - 批次大小： 一次送多少样本给模型学习
   - 梯度累积步数： 增大这个参数，相当于增大了批次大小
   - 学习率： 建议选择的范围为0.001-0.000001
   - 学习率调度器： 可供您选择的有[线性衰减、余弦衰减、多项式衰减]
   - 预热步数： 该参数的值建议为总步长的0.5%-1%，总步长=训练数据集长度 / 总批次大小 * 训练轮数
   - LoRA秩 (r)：建议其值为[8,16,32,64]，值越大模型学习的越好，但显存会随之增加。
   - LoRA Alpha： 建议其值为r的二倍
   - LoRA Dropout：建议为默认值
   - 日志记录步数： 多少个步长打印一次损失
   - 保存检查点步数： 多少个步长保存一次模型权重
   - 最大保存检查点数： 最多保留多少个训练检查点（checkpoint），超出数量后旧的会被自动删除。
   - 优化器： 默认为AdamW
   - 训练模式： 可选[LORA, QLORA]
   - 随机种子： 默认为42
   - FP16训练： 默认使用bf16训练
   - 梯度检查点： 可以节省内存占用
   - 分布式训练： 多卡训练环境（基于DDP）

## ElianFactory支持的模型

理论上Elian-Factory支持任意LLM的微调，但我们仅仅对以下模型进行了测试，如果您在微调其他模型过程中出现任何问题，请留言您的问题。

<div align="center">

| **厂家** | **支持模型** |
| :------------: | :------------: |
| DeepSeek | DeepSeek-R1:1.5b、DeepSeek-R1:8b、DeepSeek-R1:14b |
| Qwen   | Qwen3、Qwen2.5、QwQ |

</div>

## 开发者信息

- 仓库原作者：@Elian
- 邮箱：lizimo@nuist.edu.cn
欢迎大家一起对本仓库进行共建！

## 后续改进
非常感谢您使用ElianFactory，我们诚挚的邀请您对ElianFactroy做出您使用过程中的真实评价，以便于我们后续的更新改进。以下是我们对后续版本的功能扩展：
- V1.0.1
   
   增添数据生成功能、微调模型集成到Ollama
