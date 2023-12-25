import PySimpleGUI as sg

def create_timer_window(time_per_block, num_blocks, num_questions_per_block):
    # Calculate total time and time per question in seconds
    time_per_block_seconds = time_per_block * 60
    total_time = time_per_block_seconds * num_blocks
    time_per_question = time_per_block_seconds // num_questions_per_block  # Ensure integer division for seconds

    # Define layout with increased space for question display
    layout = [
        [
            sg.Text('Question 1 out of {}'.format(num_questions_per_block), key='-QUESTION-', size=(20, 1), justification='left'),
            sg.ProgressBar(time_per_block_seconds, orientation='h', size=(20, 20), key='-PROGRESS-', expand_x=True),
            sg.Text('Block 1 out of {}'.format(num_blocks), key='-BLOCK-', size=(15, 1), justification='right'),
            sg.Text('', key='-TIMER-', size=(7, 1), justification='right'),
            sg.Button('Pause', key='-PAUSE-PLAY-', size=(6, 1)),
            sg.Button('X', key='-CANCEL-', size=(3, 1), button_color=('white', 'red'))
        ]
    ]

    # Create the window with minimal borders
    window = sg.Window('Exam Practice Timer', layout, resizable=True, finalize=True, margins=(0, 0), grab_anywhere=True, keep_on_top=True)

    # Initialize variables
    current_time = total_time
    current_question = 1
    current_block = 1
    block_time_elapsed = 0
    question_time_elapsed = 0
    paused = False

    # Update loop
    try:
        while True:
            event, values = window.read(timeout=1000 if not paused else None)
            if event in (sg.WIN_CLOSED, '-CANCEL-'):
                window.close()
                break
            elif event == '-PAUSE-PLAY-':
                paused = not paused
                window['-PAUSE-PLAY-'].update('Play' if paused else 'Pause')
            elif not paused:
                # Update timer and progress bar
                current_time -= 1
                block_time_elapsed += 1
                question_time_elapsed += 1
                window['-TIMER-'].update(f'{current_time // 60:02d}:{current_time % 60:02d}')
                window['-PROGRESS-'].update_bar(block_time_elapsed)

                # Update question number
                if question_time_elapsed >= time_per_question and current_question < num_questions_per_block:
                    current_question += 1
                    question_time_elapsed = 0
                    window['-QUESTION-'].update(f'Question {current_question} out of {num_questions_per_block}')

                # Update block number and reset progress bar at the end of each block
                if block_time_elapsed >= time_per_block_seconds:
                    block_time_elapsed = 0
                    question_time_elapsed = 0
                    current_question = 1  # Reset question counter for the new block
                    window['-PROGRESS-'].update_bar(0)
                    window['-QUESTION-'].update(f'Question {current_question} out of {num_questions_per_block}')
                    if current_block < num_blocks:
                        current_block += 1
                        window['-BLOCK-'].update(f'Block {current_block} out of {num_blocks}')
                    elif current_block == num_blocks:
                        sg.popup('All blocks completed!', keep_on_top=True, title='Timer Finished')
                        break

    except Exception as e:
        sg.popup_error('An error occurred:', e, keep_on_top=True)
    finally:
        window.close()

# Startup dialog to get user inputs
def start_dialog():
    layout = [
        [sg.Text('Time per block (minutes):'), sg.InputText(key='-TIME-PER-BLOCK-', size=(5,1))],
        [sg.Text('Number of blocks:'), sg.InputText(key='-NUM-BLOCKS-', size=(5,1))],
        [sg.Text('Number of questions per block (each):'), sg.InputText(key='-NUM-QUESTIONS-PER-BLOCK-', size=(5,1))],
        [sg.Button('Start'), sg.Button('Cancel')]
    ]

    window = sg.Window('Setup', layout)

    while True:
        event, values = window.read()
        if event in (sg.WIN_CLOSED, 'Cancel'):
            window.close()
            break
        elif event == 'Start':
            try:
                time_per_block = int(values['-TIME-PER-BLOCK-'])
                num_blocks = int(values['-NUM-BLOCKS-'])
                num_questions_per_block = int(values['-NUM-QUESTIONS-PER-BLOCK-'])
                if num_questions_per_block <= 0:  # Ensure there's at least one question per block
                    raise ValueError("Number of questions per block must be greater than 0.")
                window.close()
                create_timer_window(time_per_block, num_blocks, num_questions_per_block)
                break
            except ValueError as e:
                sg.popup('Please enter valid numbers\n' + str(e), keep_on_top=True)
    window.close()

start_dialog()
