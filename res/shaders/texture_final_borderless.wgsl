struct VertexInput {
  @location(0) position: vec2f,
  @location(1) uv: vec2f,
}

struct VertexOutput {
  @builtin(position) position: vec4f,
  @location(0) pos: vec2f,
  @location(1) uv: vec2f,
}

@group(0) @binding(0) var<uniform> viewProj: mat4x4f;

@vertex
fn vs_main(in: VertexInput) -> VertexOutput {
  let out = VertexOutput(
    viewProj * vec4f(in.position, 0.0, 1.0),
    in.position,
    in.uv,
  );

  return out;
}

@group(1) @binding(0) var texture : texture_2d<f32>;
@group(1) @binding(1) var textureSampler : sampler;

@fragment
fn fs_main(in: VertexOutput) -> @location(0) vec4f {
  var color = textureSample(texture, textureSampler, in.uv);

  // var width = 2.0f / viewProj[0][0];
  // var height = -2.0f / viewProj[1][1];

  // round corners
  // let radius = 8f;

  // var alpha: f32 = 1.0f;
  // if (in.pos.x < radius && in.pos.y < radius) {
  //   alpha = CalcAlpha(vec2f(radius, radius), in.pos, radius);

  // } else if (in.pos.x > width - radius && in.pos.y < radius) {
  //   alpha = CalcAlpha(vec2f(width - radius, radius), in.pos, radius);

  // } else if (in.pos.x < radius && in.pos.y > height - radius) {
  //   alpha = CalcAlpha(vec2f(radius, height - radius), in.pos, radius);

  // } else if (in.pos.x > width - radius && in.pos.y > height - radius) {
  //   alpha = CalcAlpha(vec2f(width - radius, height - radius), in.pos, radius);
  // }
  // color.a *= alpha;

  color = ToSrgb(color);
  color = Premult(color);
  return color;
}

fn CalcAlpha(center: vec2f, pos: vec2f, radius: f32) -> f32 {
  var alpha: f32;
  var dist = length(pos - center);
  if (dist > radius) {
    alpha = 0.0;
  } else {
    dist = radius - dist;
    alpha = smoothstep(-0.1, 0.1, dist / radius);
  }
  return alpha;
}

fn ToSrgb(color: vec4f) -> vec4f {
  return vec4f(
    pow(color.r, 1.0f / 1.8f),
    pow(color.g, 1.0f / 1.8f),
    pow(color.b, 1.0f / 1.8f),
    color.a
  );
}

fn Premult(color: vec4f) -> vec4f {
  return vec4f(color.rgb * color.a, color.a);
}

