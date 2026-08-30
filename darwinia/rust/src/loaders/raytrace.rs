use crate::loaders::Loader;

pub struct RaytraceLoader {}

impl Loader for RaytraceLoader {
    const NAME: &std::ffi::CStr = c"Raytrace";
    const CHANCE_OF_SPAWNING : u32 = 10;
}
