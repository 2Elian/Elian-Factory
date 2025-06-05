# ElianFactory: Fine-tuning large language models on Windows is easy

This project develops a tool that can perform large model fine-tuning training on Windows. The fine-tuning inference framework is developed based on the transformers library.

If you are an LLM engineer, you can build your own fine-tuning inference code.
If you are familiar with the installation and operation of Docker and Linux virtual machines, you can choose [LlamaFactory](https://www.markdownguide.org) for LLM fine-tuning and inference. But if you are a novice and only have Windows operating system, [ElianFactory](https://github.com/2Elian/Elian-Factory) is your best choice.

## Advantages of ElianFactory

- 🖥️ **Developed based on Windows system**: ElianFactory fully supports native Windows system, and can perform fine-tuning and inference of large language models without deploying Linux virtual machines.
- 🚀 **Intuitive operation page**: ElianFactory is a tool that does not require building any code. You can fine-tune and reason about LLM with simple configuration.
- 🔍 **Training support**: In ElianFactory-V1.0.0, we only support SFT training. In subsequent versions, we will integrate DPO, PPO, GRPO and other functions into ElianFactory, and will consider a more efficient fine-tuning reasoning framework.

## Demo video

<video width="600" controls>
  <source src="https://github.com/2Elian/Elian-Factory/img/video.mp4" type="video/mp4">
</video>

## Quick start to build the project

### Installation steps

1. Clone the repository:
```bash
git clone https://github.com/2Elian/Elian-Factory.git
cd Elian-Factory
```

2. Install Python dependencies:
```bash
conda create -n elianfactory python=3.10
conda activate elianfactory
pip install torch==2.4.0 torchvision==0.19.0 torchaudio==2.4.0 --index-url https://download.pytorch.org/whl/cu121
```
```bash
pip install -r requirements.txt
```

3. Install node and g++:

*[node installation tutorial](node_down.md)*

*[g++ installation tutorial](g_down.md)*

4. Build the project:
```bash
cd Elian-Factory
./build.bat
```

5. Start the server:
```bash
./llm_trainer_server.exe
```

6. Visit in the browser:
```
http://localhost:10171
```
<font color=red>Note: The above steps are only performed when you build ElianFactory for the first time. If you have completed the above steps, you only need to click llm_trainer_server.exe for subsequent use. </font>

## Usage tutorial

### Model download
You can download the large model you need to fine-tune from huggingface or modelscope. This project takes deepseek-r1:1.5b as an example for explanation:
1. Start cmd
```bash
cd Elian-Factory
cd llm
conda activate elianfactory
mkdir ./ckpt/deepseek1.5
```
2. Download model
```bash
# huggingface command
huggingface-cli download deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B --local-dir ./ckpt/deepseek1.5

# modelscope command
modelscope-cli download deepseek-ai/DeepSeek-R1-Distill-Qwen-1.5B --local_dir ./ckpt/deepseek1.5
```
3. Select the model in ElianFactory

Change the model path of the front-end page to: /ckpt/deepseek1.5

### Change the dataset

The datasets of this project are all in the following format:
```bash
{"conversation": [{"human": "Here is the question", "assistant": "Here is the answer"}]}

{"conversation": [{"human": "What is machine learning?", "assistant": "Machine learning is a branch of artificial intelligence that enables computer systems to automatically improve through experience. Specifically, machine learning algorithms learn information from data by using computational methods without explicit programming. Machine learning models are generally divided into supervised learning, unsupervised learning, and reinforcement learning. "}]}
```

You can build your own dataset according to the above format (note: the dataset suffix should be jsonl), and save the built dataset to the Elian-Factory/llm/data directory.

### Adjust hyperparameters

The parameters that you can choose from Elian-Factory are as follows:
- Model path: the path where the LLM that needs to be fine-tuned is stored
- Output directory: the directory where you need to place the weights of the fine-tuned model
- Training data file: You can select the dataset you need to train on the page
- Maximum sequence length: the maximum number of tokens that the model expects to output
- Number of training rounds: how many rounds the model needs to iterate
- Batch size: how many samples are sent to the model for learning at a time
- Gradient accumulation steps: Increasing this parameter is equivalent to increasing the batch size
- Learning rate: The recommended range is 0.001-0.000001
- Learning rate scheduler: You can choose [linear decay, cosine decay, polynomial decay]
- Warm-up steps: The value of this parameter is recommended to be 0.5%-1% of the total step length, total step length = training dataset length / total batch size * number of training rounds
- LoRA rank (r): The recommended value is [8,16,32,64]. The larger the value, the better the model learns, but the video memory will increase accordingly.
- LoRA Alpha: The recommended value is twice r
- LoRA Dropout: The recommended default value
- Number of logging steps: How many steps to print the loss
- Number of checkpoint save steps: How many steps to save the model weights
- Maximum number of checkpoints to save: How many training checkpoints are retained at most. If the number is exceeded, the old ones will be automatically deleted.
- Optimizer: AdamW by default
- Training mode: optional [LORA, QLORA]
- Random seed: 42 by default
- FP16 training: bf16 training by default
- Gradient checkpoint: can save memory usage
- Distributed training: Multi-card training environment (based on DDP)

## Models supported by ElianFactory

Theoretically, Elian-Factory supports fine-tuning of any LLM, but we have only tested the following models. If you have any problems in fine-tuning other models, please leave a message about your problem.

<div align="center">

| **Manufacturer** | **Supported Model** |
| :------------: | :------------: |
| DeepSeek | DeepSeek-R1:1.5b, DeepSeek-R1:8b, DeepSeek-R1:14b |
| Qwen | Qwen3, Qwen2.5, QwQ |

</div>

## Developer Information

- Original author of the repository: @Elian
- Email: lizimo@nuist.edu.cn
Welcome everyone to build this repository together!

## Subsequent Improvements
Thank you very much for using ElianFactory. We sincerely invite you to give us a true evaluation of your use of ElianFactroy so that we can update and improve it in the future. The following are our functional expansions for subsequent versions:
- V1.0.1

Add data generation function and fine-tune the model to integrate into Ollama