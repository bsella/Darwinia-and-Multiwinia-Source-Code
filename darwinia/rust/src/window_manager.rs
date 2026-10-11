use std::{
    ffi::{CStr, c_char},
    os::raw::c_void,
};

use winit::{
    application::ApplicationHandler,
    dpi::{LogicalPosition, LogicalSize},
    event_loop::{ControlFlow, EventLoop, EventLoopProxy},
    platform::x11::WindowAttributesExtX11,
    window::{Fullscreen, Window, WindowAttributes},
};

enum RequestFromCPP {
    CreateWin {
        width: i32,
        height: i32,
        windowed: bool,
        colour_depth: i32,
        refresh_rate: i32,
        z_depth: i32,
        wait_vrt: bool,
        anti_alias: bool,
        title: String,
    },
}

unsafe extern "C" {
    unsafe fn darw_GameStep();
}

struct WindowManager {
    event_proxy: Option<EventLoopProxy<RequestFromCPP>>,
    window: Option<Window>,
}

impl ApplicationHandler<RequestFromCPP> for WindowManager {
    fn resumed(&mut self, _event_loop: &winit::event_loop::ActiveEventLoop) {}

    fn window_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        window_id: winit::window::WindowId,
        event: winit::event::WindowEvent,
    ) {
        assert_eq!(self.window.as_ref().unwrap().id(), window_id);

        match event {
            winit::event::WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            _ => {}
        }
    }

    fn about_to_wait(&mut self, _event_loop: &winit::event_loop::ActiveEventLoop) {
        //if let Some(ref window) = self.window {
        //}
        unsafe { darw_GameStep() }
    }

    fn user_event(
        &mut self,
        event_loop: &winit::event_loop::ActiveEventLoop,
        event: RequestFromCPP,
    ) {
        match event {
            RequestFromCPP::CreateWin {
                width,
                height,
                windowed,
                colour_depth: _,
                refresh_rate: _,
                z_depth: _,
                wait_vrt: _,
                anti_alias: _,
                title,
            } => {
                self.window = Some(
                    event_loop
                        .create_window(
                            WindowAttributes::default()
                                .with_base_size(LogicalSize { width, height })
                                .with_title(title)
                                .with_fullscreen(if windowed {
                                    Some(Fullscreen::Borderless(None))
                                } else {
                                    None
                                }),
                        )
                        .unwrap(),
                )
            }
        }
    }
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_Create() -> *mut WindowManager {
    std::ptr::from_mut(Box::leak(Box::new(WindowManager {
        event_proxy: None,
        window: None,
    })))
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_Delete(wm: *mut WindowManager) {
    unsafe { drop(Box::from_raw(wm)) }
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_CreateWin(
    wm: *mut WindowManager,
    width: i32,
    height: i32,
    windowed: bool,
    colour_depth: i32,
    refresh_rate: i32,
    z_depth: i32,
    wait_vrt: bool,
    anti_alias: bool,
    title: *const c_char,
) -> bool {
    let _ = unsafe { &*wm }
        .event_proxy
        .as_ref()
        .unwrap()
        .send_event(RequestFromCPP::CreateWin {
            width,
            height,
            windowed,
            colour_depth,
            refresh_rate,
            z_depth,
            wait_vrt,
            anti_alias,
            title: String::from(
                unsafe { CStr::from_ptr(title) }
                    .to_owned()
                    .to_str()
                    .unwrap(),
            ),
        });
    false
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_HideWin(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_DestroyWin(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_Flip(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_PollForMessages(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_SetMousePos(wm: *mut WindowManager, x: i32, y: i32) {
    let window = unsafe { (*wm).window.as_ref().unwrap() };

    let _ = window.set_cursor_position(LogicalPosition { x, y });
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_EnsureMouseCaptured(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_EnsureMouseUncaptured(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_CaptureMouse(wm: *mut WindowManager) {
    let window = unsafe { (*wm).window.as_ref().unwrap() };
    window.set_cursor_visible(false);
    if window
        .set_cursor_grab(winit::window::CursorGrabMode::Locked)
        .is_err()
    {
        let _ = window.set_cursor_grab(winit::window::CursorGrabMode::Confined);
    }
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_UncaptureMouse(wm: *mut WindowManager) {
    let window = unsafe { (*wm).window.as_ref().unwrap() };
    let _ = window.set_cursor_grab(winit::window::CursorGrabMode::None);
    window.set_cursor_visible(false);
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_HideMousePointer(wm: *mut WindowManager) {
    unsafe { (*wm).window.as_ref().unwrap() }.set_cursor_visible(false);
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_UnhideMousePointer(wm: *mut WindowManager) {
    unsafe { (*wm).window.as_ref().unwrap() }.set_cursor_visible(true);
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_SaveDesktop(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_RestoreDesktop(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_OpenWebsite(_wm: *mut WindowManager, _url: *const c_char) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_NastySetMousePos(_wm: *mut WindowManager, _x: i32, _y: i32) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_NastyMoveMouse(_wm: *mut WindowManager, _x: i32, _y: i32) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_NastyPollForMessages(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_PreventFullscreenStartup(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_ListAllDisplayModes(_wm: *mut WindowManager) {}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_RunGameLoop(wm: *mut WindowManager) {
    let event_loop = EventLoop::<RequestFromCPP>::with_user_event()
        .build()
        .unwrap();

    // ControlFlow::Poll continuously runs the event loop, even if the OS hasn't
    // dispatched any events. This is ideal for games and similar applications.
    event_loop.set_control_flow(ControlFlow::Poll);

    unsafe { &mut *wm }.event_proxy = Some(event_loop.create_proxy());

    event_loop.run_app(unsafe { &mut *wm }).unwrap()
}

#[unsafe(no_mangle)]
extern "C" fn darw_WindowManager_Window(_wm: *mut WindowManager) -> *mut c_void {
    std::ptr::null_mut()
}
