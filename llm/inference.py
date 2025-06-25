#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2025/4/30 17:32
# @Author  : lizimo@nuist.edu.cn
# @File    : inference.py
# @Description: 推理测试 transformers

from transformers import (
    AutoModelForCausalLM,
    AutoTokenizer,
    GenerationConfig
)
import torch
def model_reasoning(model_path: str, prompt: str, max_new_tokens=2048):
    """
    单论对话的回复
    :param model_path: 模型权重地址 -->
    :param prompt: 需要询问的问题
    :return: 回复response
    """

    # 加载模型和分词器
    model_path = model_path
    tokenizer = AutoTokenizer.from_pretrained(model_path, trust_remote_code=True)
    model = AutoModelForCausalLM.from_pretrained(model_path, torch_dtype=torch.bfloat16, trust_remote_code=True,
                                                 device_map="auto")

    model.generation_config = GenerationConfig.from_pretrained(model_path)
    model.generation_config.pad_token_id = model.generation_config.eos_token_id

    messages = [
        {"role": "user",
         "content": prompt}
    ]

    input_tensor = tokenizer.apply_chat_template(messages, add_generation_prompt=True, return_tensors="pt")
    outputs = model.generate(input_tensor.to(model.device), max_new_tokens=max_new_tokens)

    result = tokenizer.decode(outputs[0][input_tensor.shape[1]:], skip_special_tokens=True)
    return result

if __name__ == '__main__':
    model_path = "./output/shuangseqiu/combin/qwen/checkpoint-200"
    prompt = "xxx"
    max_new_tokens = 4096
    response = model_reasoning(model_path, prompt,max_new_tokens)
    print(response)


