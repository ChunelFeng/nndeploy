# e_dit

> 基于nano_vllm实现，目前代码全是nano_vllm，会在此基础上做内存优化，图优化，算子优化，量化，参数统计工具，计算量工具，访存工具，计算密度工具等等

### 推理所需内存

- 参数
  - param(Embedding) = vocab_size * hidden_size = 151936 * 1024 = 155,582,464
  - 如果tie_word_embeddings为true，则param(lm_head)与param(Embedding)共享一份参数；否则param(lm_head) = vocab_size * hidden_size = 151936 * 1024 = 155,582,464
  - Qwen3DecoderLayer * 28
    - Attention = （num_attention_heads+2*num_key_value_heads）*head_dim*hidden_size + num_head*head_dim*hidden_size + 2 * head_dim = (16 + 2  * 8)*(128 * 1024) + (16  * 128*1024) + 2*128= 6291456
    - MLP = intermediate_size * 2 * hidden_size + hidden_size * intermediate_size = 3072 * 2 * 1024 + 1024 * 3072 = 9437184
    - RMSNorm = hidden_size * 2 = 1024 * 2 = 2048
  - RMSNorm = 1024  
  - 总计为：596,049,920

- buffer - ROPE - sin cos cache

- kv_cache
  - input_ids = [batch, seq_len]
  - batch * seq_len * num_hidden_layers
    - 2 * num_key_value * head_dim

- 激活值 - 同时必须存在的Tensor
  - 从整个模型来看，必须存在三个及以上的激活值
    - 从Qwen3DecoderLayer来看，必须存在四个及以上的激活值
      - 从Attention来看，必须存在5个及以上的激活值
      - 从MLP来看，必须存在两个及以上的激活值

- 推理所需总内存：主要由参数、kv_cache决定，以及少量的激活值和Buffer

### 训练所需内存
- 参数
- buffer
- 全部的激活值（更具模型结构来的）
- 参数梯度得长久保存
- 激活值梯度所需内存，在反向传播时，梯度一边申请，一边释放已经不再被使用得激活值和梯度
- 优化器所需内存（通常等于 参数 * 2）

