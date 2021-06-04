import logging

def confirm_on_lablel(event, buttons, expected_txn_labels, confirm_label, messages_seen_history):
    logging.warning(event)
    # we have only one text
    if type(event) == dict:
        label = event['text'].lower()
        y = int(event['y'])
    elif type(event) == list:
        label = sorted(event, key=lambda e: e['y'])[0]['text'].lower()
        y = int(sorted(event, key=lambda e: e['y'])[0]['y'])
    else:
        raise Exception(f"enexpceted events type is {type(event)}")

    messages_seen_history.append([label,y])
    logging.warning('label => %s' % label)
    if len(list(filter(lambda l: l in label, expected_txn_labels))) > 0:
        if label == confirm_label:
            buttons.press(buttons.RIGHT, buttons.LEFT, buttons.RIGHT_RELEASE, buttons.LEFT_RELEASE)
        else:
            buttons.press(buttons.RIGHT, buttons.RIGHT_RELEASE)
    return messages_seen_history