// GLRenderable.cpp

bool GLRenderable::draw( scene* s ) {
  if ( frustum_cull( &bb, s->cam->frustum ) )
    return false;

  // TODO - use a cached drawcall
  drawCall* draw = drawCall::create(
    &renderPass_main,
    *shader,
    elementCount,
    null, // use VBO
    null, // use EBO
    texture,
    modelview );
  draw->texture_b = textureB;
  draw->texture_c = textureC;
  draw->texture_d = textureD;
  draw->texture_normal = terrain_texture->gl_tex;
  draw->texture_b_normal = terrain_texture->gl_tex;
  draw->vertex_VBO = vertexBufferObject;
  draw->element_VBO = elementBufferObject;

  drawCall* drawDepth = drawCall::create(
    &renderPass_depth,
    *Shader::depth(),
    elementCount,
    null, // use VBO
    null, // use EBO
    texture,
    modelview );
  drawDepth->vertex_VBO = vertexBufferObject;
  drawDepth->element_VBO = elementBufferObject;
  return true;
}
