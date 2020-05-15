#ifndef DIFIELDFWD_H
#define DIFIELDFWD_H

#include <memory>
#include <vector>

class Field;
typedef std::shared_ptr<Field> Field_p;
typedef std::shared_ptr<const Field> Field_cp;
typedef std::vector<Field_p> Field_pv;

#endif // DIFIELDFWD_H
