device_backend_flags = {
  "cpp": ["-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_CPP"],
  "omp": ["-fopenmp", "-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP"],
  "tbb": ["-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_TBB"]
}

device_backend_libs = {
  "cpp": [""],
  "omp": ["gomp"],
  "tbb": ["tbb"]
}

vars = Variables()
vars.Add(ListVariable('device_backend', 'The parallel device backend to target', 'cpp', ['cpp', 'omp', 'tbb']))
env = Environment(variables = vars)
device_backend = str(env['device_backend'])

env.Program("test.cpp", CPPPATH = ["/home/jhoberock/dev/git/thrust-tot"], CPPFLAGS = ["-std=c++0x", "-O3"] + device_backend_flags[device_backend], LIBS = device_backend_libs[device_backend])
