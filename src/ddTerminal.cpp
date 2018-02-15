#include "ddTerminal.h"
#include <regex>
#include "ddFileIO.h"

#define TOTAL_ENTRIES 1000
#define VISIBLE_ENTRIES 500
#define DEFAULT_ENTRY_SIZE 256
#define CMD_BUFFER_SIZE 10
#define CMD_HIST_SIZE 100
typedef char buffEntry[DEFAULT_ENTRY_SIZE];

ImColor colorCodeOutput(const char* entry);
void execTerminalCommand(const char* command);
int terminalCallback(ImGuiTextEditCallbackData* data);

namespace {
bool filter_ON = false;
bool RENDER_ON = false;
bool set_buffer_pos = false;
bool write_out_history = false;

buffEntry history[TOTAL_ENTRIES];
unsigned logged_input[VISIBLE_ENTRIES];
unsigned terminal_idx = 0;
unsigned entry_idx = 0;

buffEntry cmd_history[CMD_HIST_SIZE];
buffEntry cmdBuffer[CMD_BUFFER_SIZE];
buffEntry cmd_input;
unsigned cmd_buff_count = 0;
unsigned cmd_hist_head = 0;
unsigned cmd_hist_tail = 0;
unsigned cmd_scroll_idx = 0;
unsigned total_cmds_entered = 0;
bool up_pressed = false;
bool down_pressed = false;
bool tab_pressed = false;

ImGuiTextFilter filter;
float last_button_press = 0.f;

std::regex regex_str;
char regex_scratch[DEFAULT_ENTRY_SIZE];
bool restart_reg_search = true;
unsigned num_regex_matches = 0;
unsigned last_tabbed = 0;
unsigned matched_expr[CMD_HIST_SIZE];

DD_FuncBuff fb;
}  // namespace

void ddTerminal::flipDebugFlag() { RENDER_ON ^= 1; }

/// \brief Post to terminal (currently not thread safe)
void ddTerminal::post(const char* message) {
  snprintf(history[terminal_idx], DEFAULT_ENTRY_SIZE, ">>: %s", message);
  logged_input[entry_idx] = terminal_idx;
  terminal_idx = (terminal_idx + 1) % TOTAL_ENTRIES;
  entry_idx = (entry_idx + 1) % VISIBLE_ENTRIES;
  set_buffer_pos = true;
}

void ddTerminal::clearTerminal() { entry_idx = 0; }

void ddTerminal::display(const float scr_width, const float scr_height) {
  ImGuiWindowFlags window_flags = 0;
  // window_flags |= ImGuiWindowFlags_NoTitleBar;
  // window_flags |= ImGuiWindowFlags_NoResize;
  // window_flags |= ImGuiWindowFlags_NoMove;

  // const float debugY = scr_height - 150.f;
  // const float debugY = 0;
  // const float debugW = scr_width - 10.f;

  if (RENDER_ON) {
    ImGui::Begin("ddTerminal", &RENDER_ON, window_flags);

    // time information
    ImGui::Separator();
    float ft = ddTime::get_avg_frame_time();
    float fps = 1.f / ft;
    float gt = ddTime::get_time_float();
    ImGui::Text("Frame time: %.5f (%.1f FPS) || UpTime: %.5f", ft, fps, gt);

    // commandline
    ImGui::Separator();
    const char* msg = (!filter_ON) ? "Filter history" : "Enter commands";
    if (ImGui::Button(msg)) {
      filter_ON ^= 1;
      set_buffer_pos = !filter_ON;
    }
    ImGui::SameLine();

    if (filter_ON) {
      // use filter on history
      filter.Draw("Filter (\"incl,-excl\")");
    } else {
      // read in commands
      if (ImGui::InputText("Input ($<command>)", cmd_input, DEFAULT_ENTRY_SIZE,
                           ImGuiInputTextFlags_EnterReturnsTrue |
                               ImGuiInputTextFlags_CallbackCompletion |
                               ImGuiInputTextFlags_CallbackHistory,
                           &terminalCallback)) {
        if (*cmd_input) {
          execTerminalCommand(cmd_input);
        }
      }
      // keep focus on imgui
      // if (ImGui::IsItemHovered() ||
      //    (ImGui::IsRootWindowOrAnyChildFocused() &&
      //     !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0))) {
      //  ImGui::SetKeyboardFocusHere(-1);  // Auto focus previous widget
      //}
    }
    ImGui::Separator();

    ImGui::BeginChild("Terminal_ScrollingRegion");
    if (filter_ON) {
      // loop through history
      const unsigned limit =
          (terminal_idx < VISIBLE_ENTRIES) ? terminal_idx : VISIBLE_ENTRIES;
      for (unsigned i = terminal_idx - limit; i < limit; i++) {
        if (filter.PassFilter(history[i])) {
          // change color for certain text
          ImColor col = colorCodeOutput(history[i]);

          ImGui::PushStyleColor(ImGuiCol_Text, col);
          ImGui::TextWrapped(history[i]);
          ImGui::PopStyleColor();
        }
      }
    } else {
      // Refill terminal buffer
      const unsigned limit =
          (terminal_idx < VISIBLE_ENTRIES) ? entry_idx : VISIBLE_ENTRIES;
      for (unsigned i = entry_idx; i < limit; i++) {
        // change color for certain text
        ImColor col = colorCodeOutput(history[logged_input[i]]);

        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextWrapped(history[logged_input[i]]);
        ImGui::PopStyleColor();
      }
      for (unsigned i = 0; i < entry_idx; i++) {
        ImColor col = colorCodeOutput(history[logged_input[i]]);

        ImGui::PushStyleColor(ImGuiCol_Text, col);
        ImGui::TextWrapped(history[logged_input[i]]);
        ImGui::PopStyleColor();
      }
    }

    if (set_buffer_pos) {
      ImGui::SetScrollHere();  // set to end when new text shows
      set_buffer_pos = false;
    }

    ImGui::EndChild();

    ImGui::End();
  }
}

void ddTerminal::dumpTerminalToImGuiText() {
  const unsigned limit =
      (terminal_idx < VISIBLE_ENTRIES) ? entry_idx : VISIBLE_ENTRIES;
  for (unsigned i = entry_idx; i < limit; i++) {
    ImGui::TextWrapped(history[logged_input[i]]);
  }
  for (unsigned i = 0; i < entry_idx; i++) {
    ImGui::TextWrapped(history[logged_input[i]]);
  }
}

const char* ddTerminal::pollBuffer() {
  if (cmd_buff_count == 0) {
    return nullptr;
  }

  cmd_buff_count -= 1;
  return cmdBuffer[cmd_buff_count];
}

/*
DD_Event ddTerminal::getInput(DD_Event& event) {
  if (event.m_type == "input") {
    // slight delay between taps
    last_button_press += event.m_time;

    inputBuff* input = (inputBuff*)event.m_message;
    if (input->rawInput[DD_Keys::UP_KEY] && last_button_press > 0.15f) {
      up_pressed = true;
      last_button_press = 0.f;
    }
    if (input->rawInput[DD_Keys::DOWN_KEY] && last_button_press > 0.15f) {
      down_pressed = true;
      last_button_press = 0.f;
    }
    if (input->rawInput[DD_Keys::TAB_Key] && last_button_press > 0.15f) {
      tab_pressed = true;
      last_button_press = 0.f;
    }
  }
  return DD_Event();
}
//*/

void ddTerminal::get_input(DD_LEvent& _event) {
  InputData idata = ddInput::get_input();

  last_button_press += ddTime::get_frame_time();
  if (idata.keys[DD_Keys::TILDE].active && last_button_press > 0.15f) {
    RENDER_ON ^= 1;
    last_button_press = 0.f;
  }
  if (idata.keys[DD_Keys::UP_KEY].active && last_button_press > 0.15f) {
    up_pressed = true;
    last_button_press = 0.f;
  }
  if (idata.keys[DD_Keys::DOWN_KEY].active && last_button_press > 0.15f) {
    down_pressed = true;
    last_button_press = 0.f;
  }
  if (idata.keys[DD_Keys::TAB_Key].active && last_button_press > 0.15f) {
    tab_pressed = true;
    last_button_press = 0.f;
  }
}

void ddTerminal::inTerminalHistory() {
  write_out_history = true;  // prevents wiping terminal history on exit
  // read saved terminal history
  ddIO io_handle;
  cbuff<256> infile;
  infile.format("%s/%s", RESOURCE_DIR, "terminal_history.txt");
  if (!io_handle.open(infile.str(), ddIOflag::READ)) {
    return;
  }
  const char* line = io_handle.readNextLine();

  // save commands, update head and tail
  while (line && total_cmds_entered < CMD_HIST_SIZE) {
    snprintf(cmd_history[cmd_hist_tail], DEFAULT_ENTRY_SIZE, line);
    total_cmds_entered += 1;
    cmd_hist_tail += 1;

    line = io_handle.readNextLine();
  }
  cmd_hist_tail = (total_cmds_entered) % CMD_HIST_SIZE;
  cmd_scroll_idx = cmd_hist_tail;
  cmd_hist_head = (cmd_hist_head == cmd_hist_tail)
                      ? (cmd_hist_tail + 1) % CMD_HIST_SIZE
                      : 0;
}

void ddTerminal::outTerminalHistory() {
  // should only write out if history has been attempted to be read in
  // prevents wiping history on engine exit
  if (!write_out_history) {
    return;
  }

  // write out terminal history
  ddIO io_handle;
  cbuff<256> outfile;
  outfile.format("%s/%s", RESOURCE_DIR, "terminal_history.txt");
  if (!io_handle.open(outfile.str(), ddIOflag::WRITE)) {
    return;
  }

  // split history size in 2
  // If head --> tail == max history size, need to scroll thru history twice
  // to preserve ordering
  unsigned history_size_0 = 0;
  unsigned history_size_1 = 0;

  history_size_0 = (total_cmds_entered >= CMD_HIST_SIZE)
                       ? CMD_HIST_SIZE - cmd_hist_head
                       : total_cmds_entered;
  history_size_1 = (total_cmds_entered >= CMD_HIST_SIZE) ? cmd_hist_tail : 0;

  // first loop (head to end/tail)
  for (unsigned i = cmd_hist_head; i < (history_size_0 + cmd_hist_head); i++) {
    if (i != cmd_hist_head) {
      io_handle.writeLine("\n");
    }
    io_handle.writeLine(cmd_history[i]);
  }
  // second loop (only loops if total_cmds_entered >= CMD_HIST_SIZE)
  for (unsigned i = 0; i < history_size_1; i++) {
    io_handle.writeLine("\n");
    io_handle.writeLine(cmd_history[i]);
  }
}

ImColor colorCodeOutput(const char* entry) {
  ImColor col = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
  if (strstr(entry, "[error]")) {
    col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
  } else if (strstr(entry, "[status]")) {
    col = ImColor(0.4f, 1.0f, 0.4f, 1.0f);
  }
  return col;
}

void execTerminalCommand(const char* command) {
  // separate commands and add to buffer
  dd_array<cbuff<DEFAULT_ENTRY_SIZE>> cmds =
      StrSpace::tokenize1024<DEFAULT_ENTRY_SIZE>(command, "$");

  for (int i = ((int)cmds.size() - 1); i >= 0; i--) {
    if (*cmds[i].str()) {
      if (cmd_buff_count < CMD_BUFFER_SIZE) {
        char* cmd_arg = cmdBuffer[cmd_buff_count];

        snprintf(cmd_arg, DEFAULT_ENTRY_SIZE, "%s", cmds[i].str());
        cmd_buff_count += 1;
        ddTerminal::post(cmd_arg);
      }
    }
  }
  // maintain cmd history
  snprintf(cmd_history[cmd_hist_tail], DEFAULT_ENTRY_SIZE, command);
  total_cmds_entered += 1;

  cmd_hist_tail = (cmd_hist_tail + 1) % CMD_HIST_SIZE;
  cmd_hist_head = (cmd_hist_tail == cmd_hist_head)
                      ? (cmd_hist_head + 1) % CMD_HIST_SIZE
                      : cmd_hist_head;
  cmd_scroll_idx = cmd_hist_tail;

  *cmd_input = '\0';          // clear cmd line
  restart_reg_search = true;  // restart tab completion
}

int terminalCallback(ImGuiTextEditCallbackData* data) {
  bool end_of_history = false;
  if (up_pressed) {
    if (total_cmds_entered >= CMD_HIST_SIZE) {  // loop back to end
      if ((cmd_scroll_idx != cmd_hist_head)) {
        cmd_scroll_idx =
            (cmd_scroll_idx == 0) ? CMD_HIST_SIZE - 1 : cmd_scroll_idx - 1;
      }
    } else {  // stop at head
      cmd_scroll_idx = (cmd_scroll_idx == 0) ? 0 : cmd_scroll_idx - 1;
    }
    up_pressed = false;
  } else if (down_pressed) {
    if (total_cmds_entered >= CMD_HIST_SIZE) {  // loop back to top
      if ((cmd_scroll_idx != cmd_hist_tail)) {
        cmd_scroll_idx = (cmd_scroll_idx + 1) % CMD_HIST_SIZE;
      } else {
        end_of_history = true;
      }
    } else {  // stop at head
      cmd_scroll_idx += (cmd_scroll_idx == cmd_hist_tail) ? 0 : 1;
    }
    down_pressed = false;
  } else if (tab_pressed) {
    tab_pressed = false;

    if (restart_reg_search) {
      num_regex_matches = 0;
      last_tabbed = 0;
      snprintf(regex_scratch, DEFAULT_ENTRY_SIZE, "^%s.*", data->Buf);
      regex_str.assign(regex_scratch);

      // log regex matches (can be done in parallel)
      for (unsigned i = 0; i < CMD_HIST_SIZE; i++) {
        if (std::regex_search(cmd_history[i], regex_str)) {
          matched_expr[num_regex_matches] = i;
          num_regex_matches += 1;
        }
      }
      restart_reg_search = false;
    }

    if (num_regex_matches > 0) {
      unsigned t_idx = 0;
      if (last_tabbed < num_regex_matches - 1) {  // set idx to the next one
        last_tabbed += 1;
        t_idx = matched_expr[last_tabbed];
      } else {  // restart
        last_tabbed = 0;
        t_idx = matched_expr[last_tabbed];
      }
      // set tabbed text

      data->BufTextLen =
          (int)snprintf(data->Buf, data->BufSize, "%s", cmd_history[t_idx]);
      data->CursorPos = data->SelectionStart = data->SelectionEnd =
          data->BufTextLen;
      data->BufDirty = true;
    }
    return 0;  // exit after setting text
  }
  // set scrolled to text
  const char* text_ = (end_of_history) ? "" : cmd_history[cmd_scroll_idx];
  data->BufTextLen = (int)snprintf(data->Buf, data->BufSize, "%s", text_);
  data->CursorPos = data->SelectionStart = data->SelectionEnd =
      data->BufTextLen;
  data->BufDirty = true;

  return 0;
}
