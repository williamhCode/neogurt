struct VertexInput {
  @location(0) position: vec2f,
  @location(1) color: vec4f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) color: vec4f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.color,
  );

  return out;
}

struct FragmentInput {
  @builtin(position) position: vec4f,
  @location(0) color: vec4f,
}

@group(1) @binding(0) var maskTexture: texture_2d<f32>;

@fragment
fn fs_main(in: FragmentInput) -> @location(0) vec4f {
  let mask = textureLoad(maskTexture, vec2u(in.position.xy), 0).r;

  var color = in.color;
  color.a = 1.0 - mask;
  color = Blend(color, vec4f(0, 0, 0, 1));

  return color;
}

fn Blend(src: vec4f, dst: vec4f) -> vec4f {
  let outAlpha = src.a + dst.a * (1.0 - src.a);
  let outColor = src.rgb * src.a + dst.rgb * (1.0 - src.a);

  return vec4f(outColor, outAlpha);
}
