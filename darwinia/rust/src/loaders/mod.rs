use std::ffi::{CStr, c_char};

mod amiga;
mod credits;
mod fodder;
mod gameoflife;
mod matrix;
mod raytrace;
mod soul;
mod spectrum;

use amiga::AmigaLoader;
use credits::CreditsLoader;
use fodder::FodderLoader;
use gameoflife::{GameOfLifeGlowLoader, GameOfLifeLoader};
use matrix::MatrixLoader;
use rand::random_range;
use raytrace::RaytraceLoader;
use soul::SoulLoader;
use spectrum::SpectrumLoader;

const TYPE_SPECCY_LOADER: i32 = 0;
const TYPE_MATRIX_LOADER: i32 = 1;
const TYPE_FODDER_LOADER: i32 = 2;
const TYPE_RAY_TRACE_LOADER: i32 = 3;
const TYPE_SOUL_LOADER: i32 = 4;
const TYPE_GAME_OF_LIFE_LOADER: i32 = 5;
const TYPE_GAME_OF_LIFE_LOADER_GLOW: i32 = 6;
const TYPE_CREDITS_LOADER: i32 = 7;
const TYPE_AMIGA_LOADER: i32 = 8;

trait Loader {
    const NAME: &CStr;
    const CHANCE_OF_SPAWNING: u32;
}

#[unsafe(no_mangle)]
extern "C" fn darw_GetLoaderName(index: i32) -> *const c_char {
    match index {
        TYPE_SPECCY_LOADER => SpectrumLoader::NAME.as_ptr(),
        TYPE_MATRIX_LOADER => MatrixLoader::NAME.as_ptr(),
        TYPE_FODDER_LOADER => FodderLoader::NAME.as_ptr(),
        TYPE_RAY_TRACE_LOADER => RaytraceLoader::NAME.as_ptr(),
        TYPE_SOUL_LOADER => SoulLoader::NAME.as_ptr(),
        TYPE_GAME_OF_LIFE_LOADER => GameOfLifeLoader::NAME.as_ptr(),
        TYPE_GAME_OF_LIFE_LOADER_GLOW => GameOfLifeGlowLoader::NAME.as_ptr(),
        TYPE_CREDITS_LOADER => CreditsLoader::NAME.as_ptr(),
        TYPE_AMIGA_LOADER => AmigaLoader::NAME.as_ptr(),
        _ => std::ptr::null(),
    }
}

#[unsafe(no_mangle)]
extern "C" fn darw_GetLoaderIndex(name: *const c_char) -> i32 {
    let name = unsafe { CStr::from_ptr(name) };
    if name == c"random" {
        let loader_chances = [
            SpectrumLoader::CHANCE_OF_SPAWNING,
            MatrixLoader::CHANCE_OF_SPAWNING,
            FodderLoader::CHANCE_OF_SPAWNING,
            RaytraceLoader::CHANCE_OF_SPAWNING,
            SoulLoader::CHANCE_OF_SPAWNING,
            GameOfLifeLoader::CHANCE_OF_SPAWNING,
            GameOfLifeGlowLoader::CHANCE_OF_SPAWNING,
            CreditsLoader::CHANCE_OF_SPAWNING,
            AmigaLoader::CHANCE_OF_SPAWNING,
        ];

        let total_chance = loader_chances.iter().sum();

        let chosen_chance = random_range(0..total_chance);

        let mut current_chance = 0;
        for (index, chance) in loader_chances.iter().enumerate() {
            current_chance += chance;
            if chosen_chance < current_chance {
                return index as i32;
            }
        }
    }

    if name == SpectrumLoader::NAME {
        return TYPE_SPECCY_LOADER;
    }
    if name == MatrixLoader::NAME {
        return TYPE_MATRIX_LOADER;
    }
    if name == FodderLoader::NAME {
        return TYPE_FODDER_LOADER;
    }
    if name == RaytraceLoader::NAME {
        return TYPE_RAY_TRACE_LOADER;
    }
    if name == SoulLoader::NAME {
        return TYPE_SOUL_LOADER;
    }
    if name == GameOfLifeLoader::NAME {
        return TYPE_GAME_OF_LIFE_LOADER;
    }
    if name == GameOfLifeGlowLoader::NAME {
        return TYPE_GAME_OF_LIFE_LOADER_GLOW;
    }
    if name == CreditsLoader::NAME {
        return TYPE_CREDITS_LOADER;
    }
    if name == AmigaLoader::NAME {
        return TYPE_AMIGA_LOADER;
    }

    return -1;
}
