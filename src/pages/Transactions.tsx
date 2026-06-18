import { useState, useEffect } from 'react';
import { DataTable, TableContainer, Table, TableHead, TableRow, TableHeader, TableBody, TableCell } from '@carbon/react';

interface Transaction {
  id: string;
  amount: string;
  created_at: string;
  type: 'sent' | 'received';
  counterparty: string;
}

export function Transactions() {
  const [transactions, setTransactions] = useState<Transaction[]>([]);


  useEffect(() => {
    const fetchTransactions = async () => {
      try {
        const res = await fetch('/api/transactions', {
          credentials: 'same-origin'
        });
        if (res.ok) {
          const data = await res.json();
          setTransactions(data.transactions);
        }
      } catch (e) {
        console.error('Failed to fetch transactions');
      }
    };
    fetchTransactions();
  }, []);

  const headers = [
    { key: 'created_at', header: 'Date' },
    { key: 'type', header: 'Type' },
    { key: 'counterparty', header: 'Counterparty' },
    { key: 'amount', header: 'Amount ($)' }
  ];

  return (
    <div>
      <h2 style={{ marginBottom: '2rem', color: '#161616' }}>Transaction History</h2>
      <div className="history-card" style={{ backgroundColor: '#fff', padding: '2rem', borderRadius: '8px' }}>
        <DataTable rows={transactions} headers={headers}>
          {({ rows, headers, getHeaderProps, getRowProps, getTableProps }) => (
            <TableContainer>
              <Table {...getTableProps()}>
                <TableHead>
                  <TableRow>
                    {headers.map((header) => (
                      <TableHeader {...getHeaderProps({ header })}>
                        {header.header}
                      </TableHeader>
                    ))}
                  </TableRow>
                </TableHead>
                <TableBody>
                  {rows.map((row) => (
                    <TableRow {...getRowProps({ row })}>
                      {row.cells.map((cell) => (
                        <TableCell key={cell.id}>
                          {cell.info.header === 'amount' 
                            ? `$${parseFloat(cell.value).toFixed(2)}` 
                            : cell.info.header === 'created_at' 
                              ? new Date(cell.value).toLocaleString() 
                              : cell.value}
                        </TableCell>
                      ))}
                    </TableRow>
                  ))}
                </TableBody>
              </Table>
            </TableContainer>
          )}
        </DataTable>
      </div>
    </div>
  );
}
