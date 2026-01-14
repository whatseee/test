#ifndef __GATEWAY_H__
#define __GATEWAY_H__

#ifdef GATEWAY_SELECT
#define ADD_WRITE(z) \
        { \
          if (!(z->write_parent)) \
             { \
                 ((tp_Socket *) z)->write_next = ((tp_Socket *)(z->thread->write_socks)); \
                if (z->write_next) \
                  ((tp_Socket *) z)->write_next->write_prev = (tp_Socket *) z; \
                ((tp_Socket *) z)->write_prev = (tp_Socket *) 0x0; \
                ((tp_Socket *) z)->thread->write_socks = (tp_Socket *) z; \
                ((tp_Socket *) z)->write_parent = (caddr_t *)&(z->thread->write_socks);\
             } \
        }

#define ADD_READ(z) \
        { \
          if (!(z->read_parent)) \
             { \
                ((tp_Socket *) z)->read_next = ((tp_Socket *)(z->thread->read_socks)); \
                if (z->read_next) \
                  ((tp_Socket *) z)->read_next->read_prev = (tp_Socket *) z; \
                ((tp_Socket *) z)->read_prev = (tp_Socket *) (0x0); \
                ((tp_Socket *) z)->thread->read_socks = ((tp_Socket *) z); \
                ((tp_Socket *) z)->read_parent = (caddr_t *)&(z->thread->read_socks);\
             } \
        }

#define REMOVE_READ(z) \
        { \
          if (z->read_parent) \
           { \
              if (z->read_prev) \
               {\
                  ((tp_Socket *) z)->read_prev->read_next = ((tp_Socket *)z)->read_next; \
               }\
              if (z->read_next) \
                { \
                    ((tp_Socket *) z)->read_next->read_prev = (tp_Socket *)z->read_prev; \
                }\
              if ( ((tp_Socket *) z)->thread->read_socks == (tp_Socket *)z) \
                 { \
                   ((tp_Socket *) z)->thread->read_socks = (tp_Socket *)z->read_next; \
                 } \
               ((tp_Socket *) z)->read_prev = (tp_Socket *) (0x0); \
               ((tp_Socket *) z)->read_next = (tp_Socket *) (0x0); \
               ((tp_Socket *) z)->read_parent = (tp_Socket *)0x0; \
           } \
        }

#define REMOVE_WRITE(z) \
        { \
          if (z->write_parent) \
           { \
              if (z->write_prev) \
               {\
                  ((tp_Socket *) z)->write_prev->write_next = ((tp_Socket *) z)->write_next; \
               }\
              if (z->write_next) \
                { \
                    ((tp_Socket *) z)->write_next->write_prev = ((tp_Socket *) z)->write_prev; \
                }\
              if (((tp_Socket *) z)->thread->write_socks == (tp_Socket *)z) \
                 { \
                   ((tp_Socket *) z)->thread->write_socks = (tp_Socket *)(z->write_next); \
                 } \
              ((tp_Socket *) z)->write_next =  (tp_Socket *)0x0; \
              ((tp_Socket *) z)->write_prev = (tp_Socket *)0x0; \
              ((tp_Socket *) z)->write_parent = (tp_Socket *)0x0; \
           } \
        }
#endif /* GATEWAY_SELECT */

#endif /* __GATEWAY_H__ */
