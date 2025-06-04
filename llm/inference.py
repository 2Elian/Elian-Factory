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
    # model_path = "./output/shuangseqiu/combin"
    prompt = '请预测2023-01-15的双色球。其中： [ { "time": "2022-12-22", "red": [ "02", "03", "07", "13", "20", "30" ], "blue": "10" }, { "time": "2022-12-25", "red": [ "02", "15", "16", "17", "21", "31" ], "blue": "14" }, { "time": "2022-12-27", "red": [ "02", "22", "26", "29", "32", "33" ], "blue": "14" }, { "time": "2022-12-29", "red": [ "01", "07", "11", "12", "22", "28" ], "blue": "05" }, { "time": "2023-01-01", "red": [ "09", "16", "18", "22", "28", "32" ], "blue": "02" }, { "time": "2023-01-03", "red": [ "02", "06", "10", "16", "18", "22" ], "blue": "13" }, { "time": "2023-01-05", "red": [ "05", "08", "18", "25", "30", "32" ], "blue": "06" }, { "time": "2023-01-08", "red": [ "01", "07", "15", "16", "20", "25" ], "blue": "16" }, { "time": "2023-01-10", "red": [ "02", "06", "08", "13", "14", "19" ], "blue": "16" }, { "time": "2023-01-12", "red": [ "02", "10", "12", "14", "24", "26" ], "blue": "05" } ]'
    max_new_tokens = 4096
    response = model_reasoning(model_path, prompt,max_new_tokens)
    print(response)


