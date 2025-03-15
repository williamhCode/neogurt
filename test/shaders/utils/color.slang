implementing utils;

public float4 ToLinear(float4 color) {
  return {
    pow(color.r, GAMMA),
    pow(color.g, GAMMA),
    pow(color.b, GAMMA),
    color.a
  };
}

public float4 ToSrgb(float4 color) {
  return {
    pow(color.r, 1.0 / GAMMA),
    pow(color.g, 1.0 / GAMMA),
    pow(color.b, 1.0 / GAMMA),
    color.a
  };
}

public float4 Blend(float4 src, float4 dst) {
  float outAlpha = src.a + dst.a * (1.0 - src.a);
  float3 outColor = src.rgb * src.a + dst.rgb * (1.0 - src.a);

  return float4(outColor, outAlpha);
}

