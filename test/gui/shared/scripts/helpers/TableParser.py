def hashes(data_table):
    """
    Parse the data table and return a list where each row is converted to a dict.
    Column header is used as key

    Args:
        data_table: A table from a Data Table

    Returns:
        list: a list of rows (each row as a dict)

    Example:
        Data Table:
            | a | b | c |
            | 1 | 2 | 3 |
            | 4 | 5 | 6 |
        Output:
            [{'a': '1', 'b': '2', 'c': '3'}, {'a': '4', 'b': '5', 'c': '6'}]
    """
    if not data_table:
        return []

    col_header = None
    new_table = []
    for row_idx, row in enumerate(data_table):
        if row_idx == 0:
            col_header = row
            continue
        data = {}
        for col_idx, item in enumerate(row):
            data[col_header[col_idx]] = item
        new_table.append(data)
    return new_table


def rows_hash(data_table):
    """
    Parse the data table and return a dict
    where first column is used as key and second column as value.
    MUST have exactly 2 columns

    Args:
        data_table: A table from a Data Table

    Returns:
        dict: col1 as key and col2 as value

    Example:
        Data Table:
            | a | 1 |
            | b | 2 |
            | c | 3 |
        Output:
            {'a': '1', 'b': '2', 'c': '3'}
    """
    if not data_table:
        return []
    if len(data_table[0]) != 2:
        raise ValueError(
            "Error: rowsHash can only be called on a data table where all rows have exactly two columns"
        )
    new_table = {}
    for row in data_table:
        new_table[row[0]] = row[1]
    return new_table


def rows(data_table):
    """
    Parse the data table EXCLUDING FIRST ROW
    and return a 2D list where each row is converted to a list.

    Args:
        data_table: A table from a Data Table

    Returns:
        list: a 2D list

    Example:
        Data Table:
            | a | b | c |
            | 1 | 2 | 3 |
            | 4 | 5 | 6 |
        Output:
            [['1', '2', '3'], ['4', '5', '6']]
    """
    if not data_table:
        return []
    return data_table[1:]


def raw(data_table):
    """
    Parse the data table and return a 2D list where each row is converted to a list.

    Args:
        data_table: A table from a Data Table

    Returns:
        list: a 2D list (each row as a list)

    Example:
        Data Table:
            | a | b | c |
            | 1 | 2 | 3 |
            | 4 | 5 | 6 |
        Output:
            [['a', 'b', 'c'], ['1', '2', '3'], ['4', '5', '6']]
    """

    if not data_table:
        return []
    return data_table[:]
