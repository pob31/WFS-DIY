"""Repack SOFA via netCDF4 keeping float64 (libmysofa requires it) with
shuffle + gzip-9. The IR values are exactly float32-representable, so the
float64 mantissas end in ~29 zero bits — shuffle exposes the zero bytes and
deflate removes them. Bitwise identical data, much smaller file."""
import netCDF4, numpy as np, os, sys

src_path = sys.argv[1]
dst_path = sys.argv[2]

src = netCDF4.Dataset(src_path, "r")
if os.path.exists(dst_path):
    os.remove(dst_path)
dst = netCDF4.Dataset(dst_path, "w", format="NETCDF4")

for name in src.ncattrs():
    dst.setncattr(name, src.getncattr(name))
for name, dim in src.dimensions.items():
    dst.createDimension(name, len(dim))
for name, var in src.variables.items():
    kwargs = {}
    if var.size > 512:
        kwargs = dict(zlib=True, complevel=9, shuffle=True)
        if name == "Data.IR":
            kwargs["chunksizes"] = (min(256, var.shape[0]), var.shape[1], var.shape[2])
    out = dst.createVariable(name, var.dtype, var.dimensions, **kwargs)
    for a in var.ncattrs():
        out.setncattr(a, var.getncattr(a))
    out[:] = var[:]
src.close()
dst.close()

import h5py
with h5py.File(src_path, "r") as a, h5py.File(dst_path, "r") as b:
    assert np.array_equal(a["Data.IR"][()], b["Data.IR"][()]), "IR mismatch"
    assert np.array_equal(a["SourcePosition"][()], b["SourcePosition"][()]), "pos mismatch"
print("Data.IR + SourcePosition bitwise identical (float64 preserved)")
print(f"original: {os.path.getsize(src_path)/1e6:.1f} MB  repacked: {os.path.getsize(dst_path)/1e6:.1f} MB")
