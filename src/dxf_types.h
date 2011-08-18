/** @file dxf_types.h 
 *  @brief DXF (AutoCAD Drawing Interchange Format) types
 *
 * Types
 */
#ifndef _DXF_TYPES_H_
#define _DXF_TYPES_H_

/*
If you modify this, you MUST update dxf_type_name_to_enum to match.
*/
typedef enum { dxfString2049, dxfDouble3d, dxfDouble, dxfInt16, dxfInt32,
        dxfString255, dxfInt64, dxfBoolean, dxfLong } dxf_type_t;

#endif

