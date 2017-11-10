#include "DD_Terminal.h"
#include <regex>
#pragma GCC diagnostic ignored "-Wformat-security"

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
	bool DEBUG_ON = false;
	bool set_buffer_pos = false;

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
}

void DD_Terminal::flipDebugFlag() { DEBUG_ON ^= 1; }

/// \brief Post to terminal (currently not thread safe)
void DD_Terminal::post(std::string message)
{
	snprintf(history[terminal_idx], 
			 DEFAULT_ENTRY_SIZE, 
			 ">>: %s", 
			 message.c_str());
	logged_input[entry_idx] = terminal_idx;
	terminal_idx = (terminal_idx + 1) % TOTAL_ENTRIES;
	entry_idx = (entry_idx + 1) % VISIBLE_ENTRIES;
	set_buffer_pos = true;
}

void DD_Terminal::clearTerminal()
{
	entry_idx = 0;
}

void DD_Terminal::display(const float scr_width, const float scr_height)
{
	ImGuiWindowFlags window_flags = 0;
	//window_flags |= ImGuiWindowFlags_NoTitleBar;
	//window_flags |= ImGuiWindowFlags_NoResize;
	//window_flags |= ImGuiWindowFlags_NoMove;

	//const float debugY = scr_height - 150.f;
	//const float debugY = 0;
	//const float debugW = scr_width - 10.f;

	if( DEBUG_ON ) {
		ImGui::Begin("DD_Terminal", &DEBUG_ON, window_flags);

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
		}
		else {
			// read in commands
			if (ImGui::InputText("Input ($<command>)",
								 cmd_input,
								 DEFAULT_ENTRY_SIZE,
								 ImGuiInputTextFlags_EnterReturnsTrue |
								 ImGuiInputTextFlags_CallbackCompletion |
								 ImGuiInputTextFlags_CallbackHistory,
								 &terminalCallback)) 
			{
				if (*cmd_input) { execTerminalCommand(cmd_input); }
			}
			// keep focus on imgui
			if (ImGui::IsItemHovered() ||
				(ImGui::IsRootWindowOrAnyChildFocused() &&
				 !ImGui::IsAnyItemActive() &&
				 !ImGui::IsMouseClicked(0)))
			{
				ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
			}
		}
		ImGui::Separator();


		ImGui::BeginChild("Terminal_ScrollingRegion");
		if (filter_ON) {
			// loop through history
			const unsigned limit = (terminal_idx < VISIBLE_ENTRIES) ?
				terminal_idx : VISIBLE_ENTRIES;
			for (unsigned i = terminal_idx - limit; i < limit; i++) {
				if (filter.PassFilter(history[i])) { 
					// change color for certain text
					ImColor col = colorCodeOutput(history[i]);

					ImGui::PushStyleColor(ImGuiCol_Text, col);
					ImGui::TextWrapped(history[i]);
					ImGui::PopStyleColor();
				}
			}
		}
		else {
			// Refill terminal buffer
			const unsigned limit = (terminal_idx < VISIBLE_ENTRIES) ?
				entry_idx : VISIBLE_ENTRIES;
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

		if( set_buffer_pos ) {
			ImGui::SetScrollHere(); // set to end when new text shows
			set_buffer_pos = false;
		}

		ImGui::EndChild();

		ImGui::End();
	}
}

void DD_Terminal::dumpTerminalToImGuiText()
{
	const unsigned limit = (terminal_idx < VISIBLE_ENTRIES) ? entry_idx :
		VISIBLE_ENTRIES;
	for (unsigned i = entry_idx; i < limit; i++) {
		ImGui::TextWrapped(history[logged_input[i]]);
	}
	for (unsigned i = 0; i < entry_idx; i++) {
		ImGui::TextWrapped(history[logged_input[i]]);
	}
}

const char* DD_Terminal::pollBuffer()
{
	if (cmd_buff_count == 0) { return nullptr; }

	cmd_buff_count -= 1;
	return cmdBuffer[cmd_buff_count];
}

DD_Event DD_Terminal::getInput(DD_Event & event)
{
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

void DD_Terminal::getTerminalHistory(char **& history, unsigned *& hist_size)
{
	*hist_size = total_cmds_entered;
	if (total_cmds_entered > CMD_HIST_SIZE) { *hist_size = CMD_HIST_SIZE; }	
	history = (char**)cmd_history;
}

ImColor colorCodeOutput(const char * entry)
{
	ImColor col = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
	if (strstr(entry, "[error]")) {
		col = ImColor(1.0f, 0.4f, 0.4f, 1.0f);
	}
	else if (strstr(entry, "[status]")) {
		col = ImColor(0.4f, 1.0f, 0.4f, 1.0f);
	}
	return col;
}

void execTerminalCommand(const char * command)
{
	// separate commands and add to buffer
	dd_array<cbuff<DEFAULT_ENTRY_SIZE>> cmds = 
		StrSpace::tokenize512<DEFAULT_ENTRY_SIZE>(command, "$");

	for (int i = (cmds.size() - 1); i >= 0; i--) {
		if (*cmds[i].str()) {
			if (cmd_buff_count < CMD_BUFFER_SIZE) {
				char* cmd_arg = cmdBuffer[cmd_buff_count];

				snprintf(cmd_arg, DEFAULT_ENTRY_SIZE, "%s", cmds[i].str());
				cmd_buff_count += 1;
				DD_Terminal::post(cmd_arg);
			}
		}
	}
	// maintain cmd history
	snprintf(cmd_history[cmd_hist_tail], DEFAULT_ENTRY_SIZE, command);
	total_cmds_entered += 1;

	cmd_hist_tail = (cmd_hist_tail + 1) % CMD_HIST_SIZE;
	cmd_hist_head = (cmd_hist_tail == cmd_hist_head) ?
		(cmd_hist_head + 1) % CMD_HIST_SIZE : cmd_hist_head;
	cmd_scroll_idx = cmd_hist_tail;

	*cmd_input = '\0'; // clear cmd line
	restart_reg_search = true; // restart tab completion
}

int terminalCallback(ImGuiTextEditCallbackData * data)
{
	if (up_pressed) {
		if (total_cmds_entered >= CMD_HIST_SIZE) { // loop back to end
			cmd_scroll_idx = (cmd_scroll_idx == 0) ?
				CMD_HIST_SIZE - 1 : cmd_scroll_idx - 1;
		}
		else { // stop at head
			cmd_scroll_idx = (cmd_scroll_idx == 0) ?
				0 : cmd_scroll_idx - 1;
		}
		up_pressed = false;
	}
	else if (down_pressed) {
		if (total_cmds_entered >= CMD_HIST_SIZE) { // loop back to top
			cmd_scroll_idx = (cmd_scroll_idx + 1) % CMD_HIST_SIZE;
		}
		else { // stop at head
			cmd_scroll_idx += (cmd_scroll_idx == cmd_hist_tail) ?
				0 : 1;
		}
		down_pressed = false;
	}
	else if (tab_pressed) {
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
			if (last_tabbed < num_regex_matches - 1) { // set idx to the next one
				last_tabbed += 1;
				t_idx = matched_expr[last_tabbed];
			}
			else { // restart
				last_tabbed = 0;
				t_idx = matched_expr[last_tabbed];
			}
			// set tabbed text
			data->BufTextLen = (int)snprintf(
				data->Buf, data->BufSize, "%s", cmd_history[t_idx]);
			data->CursorPos = data->SelectionStart = data->SelectionEnd =
				data->BufTextLen;
			data->BufDirty = true;
		}
		return 0; // exit after setting text
	}
	// set scrolled to text
	data->BufTextLen = (int)snprintf(
		data->Buf, data->BufSize, "%s", cmd_history[cmd_scroll_idx]);
	data->CursorPos = data->SelectionStart = data->SelectionEnd =
		data->BufTextLen;
	data->BufDirty = true;

	return 0;
}
