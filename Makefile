NAME = ft_script
CC = gcc
CFLAGS = -Wall -Wextra -Werror -pedantic
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include
CFLAGS += -I$(INCLUDE_DIR)

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/pty.c \
       $(SRC_DIR)/process.c \
       $(SRC_DIR)/io.c \
       $(SRC_DIR)/terminal.c \
       $(SRC_DIR)/signal.c \
       $(SRC_DIR)/file.c \
       $(SRC_DIR)/options.c \
       $(SRC_DIR)/utils.c

OBJS = $(SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

HEADERS = $(INCLUDE_DIR)/ft_script.h

GREEN = \033[0;32m
RED = \033[0;31m
RESET = \033[0m

all: $(NAME)

$(NAME): $(OBJS)
	@echo "$(GREEN)Linking $(NAME)...$(RESET)"
	$(CC) $(CFLAGS) -o $@ $^
	@echo "$(GREEN)Build successful!$(RESET)"
	@echo "Run with: ./$(NAME)"

$(OBJS): | $(OBJ_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

docs:
	@echo "$(GREEN)Generating man pages with Doxygen...$(RESET)"
	@doxygen Doxyfile
	@echo "$(GREEN)Adding SEE ALSO cross-references...$(RESET)"
	@cd docs/man/man3 && \
	REAL=$$(for f in *.3; do head -1 "$$f" | grep -q '^\.so' || echo "$${f%.3}"; done | tr '\n' ' ') && \
	for name in $$REAL; do \
		REFS=$$(for other in $$REAL; do \
			[ "$$other" = "$$name" ] || echo ".BR $$other (3),"; \
		done | sed '$$s/,$$//') && \
		printf ".SH SEE ALSO\n%s\n" "$$REFS" >> "$$name.3"; \
	done
	@echo "$(GREEN)Man pages written to docs/man/$(RESET)"
	@echo "Preview with: man -M docs/man ft_script.h"

html: docs
	@echo "$(GREEN)Converting man pages to HTML...$(RESET)"
	@mkdir -p docs/html
	@for f in docs/man/man3/*.3; do \
		head -1 "$$f" | grep -q '^\.so' && continue; \
		name=$$(basename "$$f" .3); \
		man2html "$$f" > "docs/html/$$name.html"; \
	done
	@echo "$(GREEN)HTML pages written to docs/html/$(RESET)"

dclean:
	@echo "$(RED)Removing generated docs...$(RESET)"
	rm -rf docs
	@echo "$(RED)Docs removed.$(RESET)"

clean:
	@echo "$(RED)Cleaning object files...$(RESET)"
	rm -rf $(OBJ_DIR)
	@echo "$(RED)Clean complete.$(RESET)"

fclean: clean dclean
	@echo "$(RED)Removing $(NAME)...$(RESET)"
	rm -f $(NAME)
	@echo "$(RED)Full clean complete.$(RESET)"

re: fclean all

test: $(NAME)
	@echo "$(GREEN)Running basic test...$(RESET)"
	@./$(NAME) -c "echo 'Hello from ft_script'" test_output.txt
	@echo "$(GREEN)Test output written to test_output.txt$(RESET)"
	@cat test_output.txt
	@rm -f test_output.txt

help:
	@echo "ft_script Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build the program (default)"
	@echo "  clean   - Remove object files"
	@echo "  fclean  - Remove object files and executable"
	@echo "  re      - Rebuild from scratch (fclean + all)"
	@echo "  test    - Run a basic test"
	@echo "  help    - Show this help message"
	@echo "  docs    - Generate man pages with Doxygen"
	@echo "  html    - Convert man pages to HTML (requires docs)"
	@echo "  dclean  - Remove generated docs"
	@echo ""
	@echo "Usage:"
	@echo "  make          # Build the program"
	@echo "  make clean    # Clean object files"
	@echo "  make fclean   # Clean everything"
	@echo "  make re       # Rebuild from scratch"

.PHONY: all clean fclean re test help docs html dclean
