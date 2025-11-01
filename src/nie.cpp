#include <format>
#include <iostream>
#include <nie.hpp>
#ifdef NIELIB_FULL_X11
#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <atomic>
#include <linux/close_range.h>
#include <unistd.h>
#include <xcb/xcb.h>
#endif

namespace nie {
  using namespace std::literals;
  NIE_EXPORT void (*fatal_function)(
      std::string_view expletive, nie::source_location location) = [](std::string_view, nie::source_location) { *(volatile char*)(0) = 0; };
  [[noreturn]] NIE_EXPORT void fatal(nie::source_location location) {
    fatal("Error"sv, location);
  }
  [[noreturn]] NIE_EXPORT void fatal(std::string_view expletive, nie::source_location location) {
    nie::logger<"nie">{}.error<"fatal">("expletive"_log = expletive, "location"_log = location);
#ifdef NIELIB_FULL_X11
    std::cerr << std::format("FATAL ERROR: {} at {}", expletive, location) << std::endl;
    static std::atomic<bool> first = false;
    if ((!first.exchange(true)) && (!fork())) {
      close_range(3, 2147483647, CLOSE_RANGE_UNSHARE);
      auto loc = std::format("{}", location).substr(0, 255);
      /* Open the connection to the X server */
      xcb_connection_t* connection = xcb_connect(NULL, NULL);

      /* Get the first screen */
      xcb_screen_t* screen = xcb_setup_roots_iterator(xcb_get_setup(connection)).data;

      /* Create black (foreground) graphic context */
      xcb_drawable_t window = screen->root;
      xcb_gcontext_t foreground = xcb_generate_id(connection);
      uint32_t mask = XCB_GC_FOREGROUND | XCB_GC_GRAPHICS_EXPOSURES | XCB_GC_BACKGROUND;
      uint32_t values[3] = {screen->black_pixel, screen->white_pixel, 0};

      xcb_create_gc(connection, foreground, window, mask, values);

      /* Create a window */
      window = xcb_generate_id(connection);

      mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
      values[0] = screen->white_pixel;
      values[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_RELEASE;

      xcb_create_window(connection, /* connection          */
          XCB_COPY_FROM_PARENT,     /* depth               */
          window,                   /* window Id           */
          screen->root,             /* parent window       */
          0,
          0, /* x, y                */
          400,
          100,                           /* width, height       */
          10,                            /* border_width        */
          XCB_WINDOW_CLASS_INPUT_OUTPUT, /* class               */
          screen->root_visual,           /* visual              */
          mask,
          values); /* masks */

      /* Map the window on the screen and flush*/
      xcb_map_window(connection, window);
      xcb_flush(connection);

      /* draw primitives */
      xcb_generic_event_t* event;
      auto fe = "FATAL ERROR"sv;
      auto fe2 = "Press ESC to close..."sv;

      while ((event = xcb_wait_for_event(connection))) {
        switch (event->response_type & ~0x80) {
        case XCB_EXPOSE:
          xcb_image_text_8(connection, fe.size(), window, foreground, 0, 16, fe.data());
          xcb_image_text_8(connection, expletive.size(), window, foreground, 0, 32, expletive.data());
          xcb_image_text_8(connection, loc.size(), window, foreground, 0, 48, loc.data());
          xcb_image_text_8(connection, fe2.size(), window, foreground, 0, 64, fe2.data());
          xcb_flush(connection);
          break;
        case XCB_KEY_RELEASE: {
          xcb_key_release_event_t* ev;
          ev = (xcb_key_release_event_t*)event;
          switch (ev->detail) {
            /* ESC */
          case 9:
            free(event);
            xcb_disconnect(connection);
            exit(0);
          }
        }
        default:
          /* Unknown event type, ignore it */
          break;
        }

        free(event);
      }

      exit(0);
    }
#else
    std::cerr << "FATAL ERROR " << expletive << " at " << location.file_name() << ":" << location.line() << std::endl;
#endif
    fatal_function(expletive, location);
    abort();
  }

  nyi::nyi(std::string text, nie::source_location location) : message(std::format("NYI at {}: {}", location, text)) {
    nie::logger<"nie">{}.warn<"nyi">("text"_log = text, "location"_log = location);
    static logger<"nyi"> nyi_logger = {};
    nyi_logger.warn<"NYI at {}: {}">("location"_log = location, "message"_log = text);
  }

} // namespace nie
