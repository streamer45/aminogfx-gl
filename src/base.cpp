#include "base.h"

using namespace v8;



ColorShader* colorShader;
TextureShader* textureShader;
GLfloat* modelView;
GLfloat* globaltx;
float window_fill_red;
float window_fill_green;
float window_fill_blue;
float window_opacity;
int width = 640;
int height = 480;


std::stack<void*> matrixStack;
int rootHandle;
std::map<int,AminoFont*> fontmap;
Nan::Callback* NODE_EVENT_CALLBACK;
std::vector<AminoNode*> rects;
std::vector<Anim*> anims;
std::vector<Update*> updates;

void scale(double x, double y){
    GLfloat scale[16];
    GLfloat temp[16];
    make_scale_matrix((float)x,(float)y, 1.0, scale);
    mul_matrix(temp, globaltx, scale);
    copy_matrix(globaltx,temp);
}

void translate(double x, double y) {
    GLfloat tr[16];
    GLfloat trans2[16];
    make_trans_matrix((float)x,(float)y,0,tr);
    mul_matrix(trans2, globaltx, tr);
    copy_matrix(globaltx,trans2);
}

void rotate(double x, double y, double z) {
    GLfloat rot[16];
    GLfloat temp[16];

    make_x_rot_matrix(x, rot);
    mul_matrix(temp, globaltx, rot);
    copy_matrix(globaltx,temp);

    make_y_rot_matrix(y, rot);
    mul_matrix(temp, globaltx, rot);
    copy_matrix(globaltx,temp);

    make_z_rot_matrix(z, rot);
    mul_matrix(temp, globaltx, rot);
    copy_matrix(globaltx,temp);
}

void save() {
    GLfloat* temp = new GLfloat[16];
    copy_matrix(temp,globaltx);
    matrixStack.push(globaltx);
    globaltx = temp;
}

void restore() {
    delete globaltx;
    globaltx = (GLfloat*)matrixStack.top();
    matrixStack.pop();
}

// --------------------------------------------------------------- add_text ---
static void add_text( vertex_buffer_t * buffer, texture_font_t * font,
               wchar_t * text, vec4 * color, vec2 * pen )
{
    size_t i;
    float r = color->red, g = color->green, b = color->blue, a = color->alpha;
    for( i=0; i<wcslen(text); ++i )
    {
        texture_glyph_t *glyph = texture_font_get_glyph( font, text[i] );
        if( glyph != NULL )
        {
            int kerning = 0;
            if( i > 0)
            {
                kerning = texture_glyph_get_kerning( glyph, text[i-1] );
            }
            pen->x += kerning;
            float x0  = ( pen->x + glyph->offset_x );
            float y0  = ( pen->y + glyph->offset_y );
            float x1  = ( x0 + glyph->width );
            float y1  = ( y0 - glyph->height );
            float s0 = glyph->s0;
            float t0 = glyph->t0;
            float s1 = glyph->s1;
            float t1 = glyph->t1;
            GLushort indices[6] = {0,1,2, 0,2,3};
            vertex_t vertices[4] = { { x0,y0,0,  s0,t0,  r,g,b,a },
                                     { x0,y1,0,  s0,t1,  r,g,b,a },
                                     { x1,y1,0,  s1,t1,  r,g,b,a },
                                     { x1,y0,0,  s1,t0,  r,g,b,a } };
            vertex_buffer_push_back( buffer, vertices, 4, indices, 6 );
            pen->x += glyph->advance_x;
        }
    }
}

void TextNode::refreshText() {
    if(fontid == INVALID) return;
    AminoFont* font = fontmap[fontid];
    std::map<int,texture_font_t*>::iterator it = font->fonts.find(fontsize);
    if(it == font->fonts.end()) {
        printf("loading size %d for font %s\n",fontsize,font->filename);
        font->fonts[fontsize] = texture_font_new(font->atlas, font->filename, fontsize);
    }


    vec2 pen = {{5,400}};
    vec4 black = {{0,1,0,1}};
    pen.x = 0;
    pen.y = 0;
    black.r = r;
    black.g = g;
    black.b = b;
    black.a = opacity;

    wchar_t *t2 = const_cast<wchar_t*>(text.c_str());
    vertex_buffer_delete(buffer);
    buffer = vertex_buffer_new( "vertex:3f,tex_coord:2f,color:4f" );
    texture_font_t *f = font->fonts[fontsize];
    assert(f);
    add_text(buffer,font->fonts[fontsize],t2,&black,&pen);
//    texture_font_delete(afont->font);
}



NAN_METHOD(node_glCreateShader) {
  int type   = info[0]->Uint32Value();
  int shader = glCreateShader(type);
  info.GetReturnValue().Set(shader);
}


NAN_METHOD(node_glShaderSource) {
  int shader   = info[0]->Uint32Value();
  int count    = info[1]->Uint32Value();
  v8::String::Utf8Value jsource(info[2]->ToString());
  const char *source = *jsource;
  glShaderSource(shader, count, &source, NULL);
}

NAN_METHOD(node_glCompileShader) {
  int shader   = info[0]->Uint32Value();
  glCompileShader(shader);
}
NAN_METHOD(node_glGetShaderiv) {
  int shader   = info[0]->Uint32Value();
  int flag   = info[1]->Uint32Value();
  GLint status;
  glGetShaderiv(shader,flag,&status);
  info.GetReturnValue().Set(status);
}
NAN_METHOD(node_glGetProgramiv) {
  int prog   = info[0]->Uint32Value();
  int flag   = info[1]->Uint32Value();
  GLint status;
  glGetProgramiv(prog,flag,&status);
  info.GetReturnValue().Set(status);
}
NAN_METHOD(node_glGetShaderInfoLog) {
  int shader   = info[0]->Uint32Value();
  char buffer[513];
  glGetShaderInfoLog(shader,512,NULL,buffer);
  info.GetReturnValue().Set(Nan::New(buffer,strlen(buffer)).ToLocalChecked());
}
NAN_METHOD(node_glGetProgramInfoLog) {
  int shader   = info[0]->Uint32Value();
  char buffer[513];
  glGetProgramInfoLog(shader,512,NULL,buffer);
  info.GetReturnValue().Set(Nan::New(buffer,strlen(buffer)).ToLocalChecked());
}
NAN_METHOD(node_glCreateProgram) {
  int prog = glCreateProgram();
  info.GetReturnValue().Set(prog);
}
NAN_METHOD(node_glAttachShader) {
  int prog     = info[0]->Uint32Value();
  int shader   = info[1]->Uint32Value();
  glAttachShader(prog,shader);
}

NAN_METHOD(node_glLinkProgram) {
    int prog     = info[0]->Uint32Value();
    glLinkProgram(prog);
}
NAN_METHOD(node_glUseProgram) {
    int prog     = info[0]->Uint32Value();
    glUseProgram(prog);
}
NAN_METHOD(node_glGetAttribLocation) {
  int prog                 = info[0]->Uint32Value();
  v8::String::Utf8Value name(info[1]->ToString());
  int loc = glGetAttribLocation(prog,*name);
  info.GetReturnValue().Set(loc);
}

NAN_METHOD(node_glGetUniformLocation) {
    int prog                 = info[0]->Uint32Value();
    v8::String::Utf8Value name(info[1]->ToString());
    int loc = glGetUniformLocation(prog,*name);
    info.GetReturnValue().Set(loc);
}

NAN_METHOD(updateProperty) {
    int rectHandle   = info[0]->Uint32Value();
    int property     = info[1]->Uint32Value();
    float value = 0;
    std::wstring wstr = L"";
    if(info[2]->IsNumber()) {
        value = info[2]->NumberValue();
        printf("  setting number %f on prop %d \n",value,property);
    }
    if(info[2]->IsString()) {
        wstr = GetWString(info[2]->ToString());
    }
    std::vector<float>* arr = NULL;
    if(info[2]->IsArray()) {
        arr = GetFloatArray(v8::Handle<v8::Array>::Cast(info[2]));
    }
    updates.push_back(new Update(RECT, rectHandle, property, value, wstr,arr));
}

NAN_METHOD(updateAnimProperty) {
    int rectHandle   = info[0]->Uint32Value();
    int property     = info[1]->Uint32Value();
    float value = 0;
    //char* cstr = "";
    std::wstring wstr = L"";
    printf("doing anim update\n");
    if(info[2]->IsNumber()) {
        value = info[2]->NumberValue();
        printf("  setting number %f on prop %d \n",value,property);
    }
    if(info[2]->IsString()) {
       printf("trying to do a string\n");
       char* cstr = TO_CHAR(info[2]);
        wstr = GetWC(cstr);
    }
    printf("pushing\n");
    updates.push_back(new Update(ANIM, rectHandle, property, value, wstr, NULL));
    printf("done pushing\n");
}

NAN_METHOD(updateWindowProperty) {
    int windowHandle   = info[0]->Uint32Value();
    int property       = info[1]->Uint32Value();
    float value = 0;
    std::wstring wstr = L"";
    if(info[2]->IsNumber()) {
        printf("updating window property %d \n",property);
        value = info[2]->Uint32Value();
    }
    if(info[2]->IsString()) {
       char* cstr = TO_CHAR(info[2]);
        wstr = GetWC(cstr);
    }
    updates.push_back(new Update(WINDOW, windowHandle, property, value, wstr, NULL));
}

NAN_METHOD(addNodeToGroup) {
    int rectHandle   = info[0]->Uint32Value();
    int groupHandle  = info[1]->Uint32Value();
    Group* group = (Group*)rects[groupHandle];
    AminoNode* node = rects[rectHandle];
    group->children.push_back(node);
}

NAN_METHOD(removeNodeFromGroup) {
    int rectHandle   = info[0]->Uint32Value();
    int groupHandle  = info[1]->Uint32Value();
    Group* group = (Group*)rects[groupHandle];
    AminoNode* node = rects[rectHandle];
    int n = -1;
    for(std::size_t i=0; i<group->children.size(); i++) {
        if(group->children[i] == node) {
            n = i;
        }
    }

    group->children.erase(group->children.begin()+n);
}

NAN_METHOD(setRoot) {
    rootHandle = info[0]->Uint32Value();
}


NAN_METHOD(decodeJpegBuffer) {
    //   if(!Buffer::HasInstance(args[0])){
    //       printf("first argument must be a buffer.\n");
    //       return scope.Close(Undefined());
    //   }

    Local<Object> bufferObj = info[0]->ToObject();
    char* bufferin = Buffer::Data(bufferObj);
    size_t bufferLength = Buffer::Length(bufferObj);


    njInit();
    if (njDecode(bufferin, bufferLength)) {
        printf("Error decoding the input file.\n");
        return;
    }

    printf("got an image %d %d\n",njGetWidth(),njGetHeight());
    printf("size = %d\n",njGetImageSize());
    int lengthout = njGetImageSize();
    char* image = (char*) njGetImage();

    MaybeLocal<Object> buff = Nan::CopyBuffer(image, lengthout);

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("w").ToLocalChecked(),      Nan::New(njGetWidth()));
    Nan::Set(obj, Nan::New("h").ToLocalChecked(),      Nan::New(njGetHeight()));
    Nan::Set(obj, Nan::New("alpha").ToLocalChecked(),  Nan::New(false));
    Nan::Set(obj, Nan::New("bpp").ToLocalChecked(),    Nan::New(3));
    Nan::Set(obj, Nan::New("buffer").ToLocalChecked(), buff.ToLocalChecked());
    info.GetReturnValue().Set(obj);

    njDone();
}

 
NAN_METHOD(decodePngBuffer) {
    //    if(!Buffer::HasInstance(args[0])){
    //        printf("first argument must be a buffer.\n");
    //        return scope.Close(Undefined());
    //    }


    Local<Object> bufferObj = info[0]->ToObject();
    char* bufferin = Buffer::Data(bufferObj);
    size_t bufferLength = Buffer::Length(bufferObj);
    //    printf("the size of the buffer is %d\n",bufferLength);



    unsigned width, height;
    unsigned char* png;
    char* image;

    upng_t* upng = upng_new_from_bytes((const unsigned char*) bufferin, bufferLength);
    if(upng == NULL) {
        printf("error decoding png file");
        return;
    }

    upng_decode(upng);
    //    printf("width = %d %d\n",upng_get_width(upng), upng_get_height(upng));
    //    printf("bytes per pixel = %d\n",upng_get_pixelsize(upng));

    image = (char*) upng_get_buffer(upng);
    int lengthout = upng_get_size(upng);
    //    printf("length of uncompressed buffer = %d\n", lengthout);

    MaybeLocal<Object> buff = Nan::CopyBuffer(image, lengthout);


    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("w").ToLocalChecked(),      Nan::New(upng_get_width(upng)));
    Nan::Set(obj, Nan::New("h").ToLocalChecked(),      Nan::New(upng_get_height(upng)));
    Nan::Set(obj, Nan::New("alpha").ToLocalChecked(),  Nan::New(true));
    Nan::Set(obj, Nan::New("bpp").ToLocalChecked(),    Nan::New(4));
    Nan::Set(obj, Nan::New("buffer").ToLocalChecked(), buff.ToLocalChecked());
    info.GetReturnValue().Set(obj);

    upng_free(upng);
}


NAN_METHOD(loadBufferToTexture) {
    int texid   = info[0]->Uint32Value();
    int w   = info[1]->Uint32Value();
    int h   = info[2]->Uint32Value();
    // this is *bytes* per pixel. usually 3 or 4
    int bpp = info[3]->Uint32Value();
    //printf("got w %d h %d\n",w,h);
    Local<Object> bufferObj = info[4]->ToObject();
    char* bufferData = Buffer::Data(bufferObj);
    size_t bufferLength = Buffer::Length(bufferObj);
    //printf("buffer length = %d\n", bufferLength);

    assert(w*h*bpp == bufferLength);

    GLuint texture;
    if(texid >= 0) {
	    texture = texid;
    } else {
	    glGenTextures(1, &texture);
    }
    glBindTexture(GL_TEXTURE_2D, texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    if(bpp == 3) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, bufferData);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, bufferData);
    }
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    v8::Local<v8::Object> obj = Nan::New<v8::Object>();
    Nan::Set(obj, Nan::New("texid").ToLocalChecked(), Nan::New(texture));
    Nan::Set(obj, Nan::New("w").ToLocalChecked(), Nan::New(w));
    Nan::Set(obj, Nan::New("h").ToLocalChecked(), Nan::New(h));
    info.GetReturnValue().Set(obj);
}
NAN_METHOD(getFontHeight) {
    int fontsize   = info[0]->Uint32Value();
    int fontindex  = info[1]->Uint32Value();
    AminoFont * font = fontmap[fontindex];
    texture_font_t *tf = font->fonts[fontsize];
    info.GetReturnValue().Set(tf->ascender-tf->descender);
}
NAN_METHOD(getFontAscender) {
    int fontsize   = info[0]->Uint32Value();
    int fontindex  = info[1]->Uint32Value();
    AminoFont * font = fontmap[fontindex];
    std::map<int,texture_font_t*>::iterator it = font->fonts.find(fontsize);
    if(it == font->fonts.end()) {
        printf("Font is missing glyphs for size %d\n",fontsize);
        printf("loading size %d for font %s\n",fontsize,font->filename);
        font->fonts[fontsize] = texture_font_new(font->atlas, font->filename, fontsize);
    }
    texture_font_t *tf = font->fonts[fontsize];
    info.GetReturnValue().Set(tf->ascender);
}
NAN_METHOD(getFontDescender) {
    int fontsize   = info[0]->Uint32Value();
    int fontindex  = info[1]->Uint32Value();
    AminoFont * font = fontmap[fontindex];
    std::map<int,texture_font_t*>::iterator it = font->fonts.find(fontsize);
    if(it == font->fonts.end()) {
        printf("Font is missing glyphs for size %d\n",fontsize);
        printf("loading size %d for font %s\n",fontsize,font->filename);
        font->fonts[fontsize] = texture_font_new(font->atlas, font->filename, fontsize);
    }
    texture_font_t *tf = font->fonts[fontsize];
    info.GetReturnValue().Set(tf->descender);
}
NAN_METHOD(getCharWidth) {
    std::wstring wstr = GetWString(info[0]->ToString());

    int fontsize  = info[1]->Uint32Value();
    int fontindex  = info[2]->Uint32Value();

    AminoFont * font = fontmap[fontindex];
    assert(font);

    std::map<int,texture_font_t*>::iterator it = font->fonts.find(fontsize);
    if(it == font->fonts.end()) {
        printf("Font is missing glyphs for size %d\n",fontsize);
        printf("loading size %d for font %s\n",fontsize,font->filename);
        font->fonts[fontsize] = texture_font_new(font->atlas, font->filename, fontsize);
    }

    texture_font_t *tf = font->fonts[fontsize];
    assert( tf );

    float w = 0;
    //length seems to include the null string
    for(std::size_t i=0; i<wstr.length(); i++) {
        wchar_t ch  = wstr.c_str()[i];
        //skip null terminators
        if(ch == '\0') continue;
        texture_glyph_t *glyph = texture_font_get_glyph(tf, wstr.c_str()[i]);
        if(glyph == 0) {
            printf("WARNING. Got empty glyph from texture_font_get_glyph");
        }
        w += glyph->advance_x;
    }
    info.GetReturnValue().Set(w);
}

NAN_METHOD(createNativeFont) {
    AminoFont* afont = new AminoFont();
    int id = fontmap.size();
    fontmap[id] = afont;

    afont->filename = TO_CHAR(info[0]);

    char* shader_base = TO_CHAR(info[1]);

    printf("loading font file %s\n",afont->filename);
    printf("shader base = %s\n",shader_base);

    std::string str ("");
    std::string str2 = str + shader_base;
    std::string vert = str2 + "/v3f-t2f-c4f.vert";
    std::string frag = str2 + "/v3f-t2f-c4f.frag";

    afont->atlas = texture_atlas_new(512,512,1);
    afont->shader = shader_load(vert.c_str(),frag.c_str());
    info.GetReturnValue().Set(id);
}

NAN_METHOD(initColorShader) {
    if(info.Length() < 5) {
        printf("initColorShader: not enough args\n");
        exit(1);
    };
    colorShader->prog        = info[0]->Uint32Value();
    colorShader->u_matrix    = info[1]->Uint32Value();
    colorShader->u_trans     = info[2]->Uint32Value();
    colorShader->u_opacity   = info[3]->Uint32Value();
    colorShader->attr_pos    = info[4]->Uint32Value();
    colorShader->attr_color  = info[5]->Uint32Value();
}

NAN_METHOD(initTextureShader) {
    if(info.Length() < 6) {
        printf("initTextureShader: not enough args\n");
        exit(1);
    };
    textureShader->prog        = info[0]->Uint32Value();
    textureShader->u_matrix    = info[1]->Uint32Value();
    textureShader->u_trans     = info[2]->Uint32Value();
    textureShader->u_opacity   = info[3]->Uint32Value();

    textureShader->attr_pos    = info[4]->Uint32Value();
    textureShader->attr_texcoords  = info[5]->Uint32Value();
    textureShader->attr_tex    = info[6]->Uint32Value();
}


NAN_METHOD(createRect) {
    Rect* rect = new Rect();
    rects.push_back(rect);
    rects.size();
    info.GetReturnValue().Set((int)rects.size()-1);
}
NAN_METHOD(createPoly) {
    PolyNode* node = new PolyNode();
    rects.push_back(node);
    info.GetReturnValue().Set((int)rects.size()-1);
}
NAN_METHOD(createText) {
    TextNode * node = new TextNode();
    rects.push_back(node);
    info.GetReturnValue().Set((int)rects.size()-1);
}
NAN_METHOD(createGroup) {
    Group* node = new Group();
    rects.push_back(node);
    info.GetReturnValue().Set((int)rects.size()-1);
}
NAN_METHOD(createGLNode) {
    GLNode* node = new GLNode();
    //node->callback = Persistent<Function>::New(Handle<Function>::Cast(args[0]));
    rects.push_back(node);
    info.GetReturnValue().Set((int)rects.size()-1);
}

NAN_METHOD(createAnim) {
    int rectHandle   = info[0]->Uint32Value();
    int property     = info[1]->Uint32Value();
    float start      = info[2]->Uint32Value();
    float end        = info[3]->Uint32Value();
    float duration   = info[4]->Uint32Value();

    Anim* anim = new Anim(rects[rectHandle],property, start,end,  duration);
    anims.push_back(anim);
    anims.size();
    anim->id = anims.size()-1;
    anim->active = true;
    info.GetReturnValue().Set((int)anims.size()-1);
}


NAN_METHOD(stopAnim) {
	int id = info[0]->Uint32Value();
	Anim* anim = anims[id];
	anim->active = false;
}
