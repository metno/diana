
#ifndef diVcrossHardcopy_h
#define diVcrossHardcopy_h 1

class FontManager;
class GLPfile;
class printOptions;

class VcrossHardcopy {
public:
  VcrossHardcopy(FontManager* fp);

  bool start(const printOptions& po);
  bool end();

  bool on() const
    { return psoutput != 0; }

  void addHCStencil(const int& size, const float* x, const float* y);
  void addHCScissor(const int x0, const int y0, const int w, const int h);
  void removeHCClipping();
  void UpdateOutput();
  void resetPage();

  bool startPSnewpage();

private:
  FontManager* fp;
  GLPfile* psoutput;
};

#endif // diVcrossHardcopy_h
