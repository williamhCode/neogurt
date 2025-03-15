implementing utils;

public struct TextureInfo {
  public Texture2D texture;
  public SamplerState sampler;

  public float4 Sample(float2 uv) {
    return texture.Sample(sampler, uv);
  }
}
