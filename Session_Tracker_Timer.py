import PySimpleGUI as sg

def create_timer_window(time_per_block, num_blocks, num_questions_per_block, transparency):
    time_per_block_seconds = time_per_block * 60
    total_time = time_per_block_seconds * num_blocks
    time_per_question = time_per_block_seconds // num_questions_per_block
    
    alpha_channel_value = transparency / 100

    layout = [
        [
            sg.Text('Question 1 out of {}'.format(num_questions_per_block), key='-QUESTION-', size=(20, 1)),
            sg.ProgressBar(time_per_question, orientation='h', size=(20, 10), key='-PROGRESS-QUESTION-', bar_color=('green', 'white'), expand_x=True),
            sg.Text('00:00', key='-TIMER-QUESTION-', size=(7, 1)),
        ],
        [
            sg.Text('Block 1 out of {}'.format(num_blocks), key='-BLOCK-', size=(15, 1)),
            sg.ProgressBar(time_per_block_seconds, orientation='h', size=(20, 20), key='-PROGRESS-BLOCK-', expand_x=True),
            sg.Text('', key='-TIMER-BLOCK-', size=(7, 1)),
            sg.Button('Pause', key='-PAUSE-Resume-', size=(6, 1)),
            sg.Button('X', key='-CANCEL-', size=(3, 1), button_color=('white', 'red'))
        ]
    ]

    window = sg.Window('Exam Practice Timer', layout, 
                        resizable=True,
                        alpha_channel=alpha_channel_value,
                        no_titlebar=False,
                        finalize=True,
                        margins=(0, 0),
                        grab_anywhere=True,
                        keep_on_top=True)

    current_time = total_time
    current_question = 1
    current_block = 1
    block_time_elapsed = 0
    question_time_elapsed = 0
    paused = False

    try:
        while True:
            event, values = window.read(timeout=1000 if not paused else None)
            if event in (sg.WIN_CLOSED, '-CANCEL-'):
                break
            elif event == '-PAUSE-Resume-':
                paused = not paused
                window['-PAUSE-Resume-'].update('Resume' if paused else 'Pause')
            elif not paused:
                current_time -= 1
                block_time_elapsed += 1
                question_time_elapsed += 1

                window['-TIMER-BLOCK-'].update(f'{current_time // 60:02d}:{current_time % 60:02d}')
                window['-PROGRESS-BLOCK-'].update_bar(block_time_elapsed)

                window['-TIMER-QUESTION-'].update(f'{question_time_elapsed // 60:02d}:{question_time_elapsed % 60:02d}')
                window['-PROGRESS-QUESTION-'].update_bar(question_time_elapsed)

                if question_time_elapsed >= time_per_question:
                    question_time_elapsed = 0
                    current_question += 1
                    if current_question <= num_questions_per_block:
                        window['-QUESTION-'].update(f'Question {current_question} out of {num_questions_per_block}')

                if block_time_elapsed >= time_per_block_seconds:
                    block_time_elapsed = 0
                    current_question = 1
                    window['-PROGRESS-QUESTION-'].update_bar(0)
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

def start_dialog():
    layout = [
        [sg.Text('Time per block (minutes):'), sg.InputText(key='-TIME-PER-BLOCK-', size=(5,1))],
        [sg.Text('Number of blocks:'), sg.InputText(key='-NUM-BLOCKS-', size=(5,1))],
        [sg.Text('Number of questions per block (each):'), sg.InputText(key='-NUM-QUESTIONS-PER-BLOCK-', size=(5,1))],
        [sg.Text('Transparency (%):'), sg.InputText(default_text='75', key='-TRANSPARENCY-', size=(5,1))],
        [sg.Button('Start'), sg.Button('Cancel')]
    ]

    window = sg.Window('Setup', layout)

    while True:
        event, values = window.read()
        if event in (sg.WIN_CLOSED, 'Cancel'):
            window.close()
            return False
        elif event == 'Start':
            try:
                time_per_block = int(values['-TIME-PER-BLOCK-'])
                num_blocks = int(values['-NUM-BLOCKS-'])
                num_questions_per_block = int(values['-NUM-QUESTIONS-PER-BLOCK-'])
                transparency = int(values['-TRANSPARENCY-'])
                if num_questions_per_block <= 0 or transparency < 0 or transparency > 100:
                    raise ValueError("Invalid input.")
                window.close()
                create_timer_window(time_per_block, num_blocks, num_questions_per_block, transparency)
                return True 
            except ValueError as e:
                sg.popup('Please enter valid numbers\n' + str(e), keep_on_top=True)

if __name__ == "__main__":
    continue_app = True
    while continue_app:
        continue_app = start_dialog()

