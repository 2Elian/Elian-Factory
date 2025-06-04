#!/usr/bin/env python
# -*- coding: utf-8 -*-
# @Time    : 2025/5/3 14:25
# @Author  : lizimo@nuist.edu.cn
# @File    : loader_transformers.py
# @Description:
def process_data(data: dict, tokenizer, max_seq_length):
    conversation = data["conversation"][0]

    human_text = conversation["human"].strip()
    assistant_text = conversation["assistant"].strip()

    # 构造模型输入格式（根据DeepSeek的模板）
    input_text = f"Human: {human_text}\n\nAssistant: "

    input_tokenizer = tokenizer(
        input_text,
        add_special_tokens=False,
        truncation=True,
        max_length=max_seq_length,
        return_tensors=None,
    )

    output_tokenizer = tokenizer(
        assistant_text,
        add_special_tokens=False,
        truncation=True,
        max_length=max_seq_length,
        return_tensors=None,
    )

    return {
        "input_ids": input_tokenizer["input_ids"] + output_tokenizer["input_ids"] + [tokenizer.eos_token_id],
        "attention_mask": input_tokenizer["attention_mask"] + output_tokenizer["attention_mask"] + [1],
        "labels": [-100] * len(input_tokenizer["input_ids"]) + output_tokenizer["input_ids"] + [tokenizer.eos_token_id]
    }