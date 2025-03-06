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
