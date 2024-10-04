#include <furi.h>
#include <gui/gui.h>
#include <input/input.h>
#include <stdlib.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <storage/storage.h> 

#define MAX_COUNT 4
#define OFFSET_Y 9
#define SAVE_FILE_PATH EXT_PATH("lishi_values.txt") 

typedef struct {
    FuriMessageQueue* input_queue;
    ViewPort* view_port;
    Gui* gui;
    FuriMutex* mutex;
    NotificationApp* notifications;

    int counts[8]; // Counter for 8 columns
    bool pressed[8]; // Press status for each column
    int boxtimer[8]; // Timer for each column
    int active_column; // Index of the active column
} Lishi;

void state_free(Lishi* l) {
    gui_remove_view_port(l->gui, l->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(l->view_port);
    furi_message_queue_free(l->input_queue);
    furi_mutex_free(l->mutex);
    free(l);
}

static void input_callback(InputEvent* input_event, void* ctx) {
    Lishi* l = ctx;
    if(input_event->type == InputTypeShort || input_event->type == InputTypeLong) {
        furi_message_queue_put(l->input_queue, input_event, 0);
    }
}

static void render_callback(Canvas* canvas, void* ctx) {
    Lishi* l = ctx;
    furi_check(furi_mutex_acquire(l->mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    
   
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 10, "LISHI HU66"); 

  
    canvas_set_font(canvas, FontSecondary); 
    for(int i = 0; i < 8; i++) {
        char dynamic_number[2];
        snprintf(dynamic_number, sizeof(dynamic_number), "%d", i + 1); 
        canvas_draw_str_aligned(canvas, 10 + (i * 15), 35, AlignCenter, AlignCenter, dynamic_number); 
    }

   
    for(int i = 0; i < 8; i++) {
        char scount[8];
        snprintf(scount, sizeof(scount), "%d", l->counts[i]);
        canvas_draw_str_aligned(canvas, 10 + (i * 15), 50, AlignCenter, AlignCenter, scount); 
    }

   
    size_t active_x = 10 + (l->active_column * 15); 
    canvas_draw_rframe(canvas, active_x - 6, 40, 12, 20, 2); 

    furi_mutex_release(l->mutex);
}

Lishi* state_init() {
    Lishi* l = malloc(sizeof(Lishi));
    l->input_queue = furi_message_queue_alloc(8, sizeof(InputEvent));
    l->view_port = view_port_alloc();
    l->gui = furi_record_open(RECORD_GUI);
    l->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    l->notifications = furi_record_open(RECORD_NOTIFICATION);
    for(int i = 0; i < 8; i++) {
        l->counts[i] = 0; 
        l->pressed[i] = false;
        l->boxtimer[i] = 0;
    }
    l->active_column = 0; 
    view_port_input_callback_set(l->view_port, input_callback, l);
    view_port_draw_callback_set(l->view_port, render_callback, l);
    gui_add_view_port(l->gui, l->view_port, GuiLayerFullscreen);
    return l;
}


void save_lishi_values(Lishi* l) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE); 
    File* file = storage_file_alloc(storage);
    if (storage_file_open(file, SAVE_FILE_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, l->counts, sizeof(l->counts)); 
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}


void load_lishi_values(Lishi* l) {
    Storage* storage = (Storage*)furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if (storage_file_open(file, SAVE_FILE_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, l->counts, sizeof(l->counts)); 
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

int32_t lishiapp(void) {
    Lishi* l = state_init();
    
  
    load_lishi_values(l);

    InputEvent input;
    for(bool processing = true; processing;) {
        while(furi_message_queue_get(l->input_queue, &input, FuriWaitForever) == FuriStatusOk) {
            furi_check(furi_mutex_acquire(l->mutex, FuriWaitForever) == FuriStatusOk);

            if(input.type == InputTypeShort) {
                switch(input.key) {
                    case InputKeyBack:
                        save_lishi_values(l); 
                        processing = false;
                        break;
                    case InputKeyRight:
                      
                        l->active_column = (l->active_column + 1) % 8;
                        break;
                    case InputKeyLeft:
                        
                        l->active_column = (l->active_column - 1 + 8) % 8;
                        break;
                    case InputKeyUp:
                     
                        if(l->counts[l->active_column] < MAX_COUNT) {
                            l->pressed[l->active_column] = true;
                            l->boxtimer[l->active_column] = 2; 
                            l->counts[l->active_column]++;
                        }
                        break;
                    case InputKeyDown:
                       
                        if(l->counts[l->active_column] > 0) {
                            l->pressed[l->active_column] = true;
                            l->boxtimer[l->active_column] = 2; 
                            l->counts[l->active_column]--;
                        }
                        break;
                    default:
                        break;
                }
            } else if(input.type == InputTypeLong) {
                switch(input.key) {
                    case InputKeyBack:
                       
                        l->counts[l->active_column] = 0;
                        break;
                    case InputKeyOk:
                        
                        save_lishi_values(l); 
                        break;
                    default:
                        break;
                }
            }
            furi_mutex_release(l->mutex);
            if(!processing) {
                break;
            }
            view_port_update(l->view_port);
        }
    }
    state_free(l);
    return 0;
}

