import { useState } from 'react'
import { Monitor, FileText, RefreshCw } from 'lucide-react'
import {
  Table, TableBody, TableCell, TableHead,
  TableHeader, TableRow,
} from '@/components/ui/table'
import { Button } from '@/components/ui/button'
import { Badge } from '@/components/ui/badge'

export default function MachineTable({ machines, onRefresh, loading }) {
  const [actionTarget, setActionTarget] = useState(null)

  const handleAction = (action, machine) => {
    setActionTarget({ action, machine })
    alert(`[${action}] triggered for ${machine.hostname}`)
  }

  if (loading) {
    return (
      <div className="flex items-center justify-center py-16 text-muted-foreground">
        <RefreshCw className="mr-2 h-4 w-4 animate-spin" />
        Loading machines...
      </div>
    )
  }

  if (machines.length === 0) {
    return (
      <div className="flex flex-col items-center justify-center py-16 text-muted-foreground gap-2">
        <Monitor className="h-8 w-8 opacity-40" />
        <p className="text-sm">No machines reporting yet.</p>
      </div>
    )
  }

  return (
    <Table>
      <TableHeader>
        <TableRow>
          <TableHead className="w-12">#</TableHead>
          <TableHead>Hostname</TableHead>
          <TableHead>OS</TableHead>
          <TableHead>Private IP</TableHead>
          <TableHead>Public IP</TableHead>
          <TableHead>Last Seen</TableHead>
          <TableHead className="text-right">Actions</TableHead>
        </TableRow>
      </TableHeader>
      <TableBody>
        {machines.map((machine) => (
          <TableRow key={machine.id}>
            <TableCell className="text-muted-foreground">{machine.id}</TableCell>
            <TableCell className="font-medium">{machine.hostname}</TableCell>
            <TableCell>
              <Badge variant="secondary">{machine.os_info}</Badge>
            </TableCell>
            <TableCell className="font-mono text-xs">{machine.private_ip ?? '—'}</TableCell>
            <TableCell className="font-mono text-xs">{machine.public_ip ?? '—'}</TableCell>
            <TableCell className="text-xs text-muted-foreground">
              {machine.last_seen
                ? new Date(machine.last_seen).toLocaleString()
                : '—'}
            </TableCell>
            <TableCell className="text-right">
              <div className="flex justify-end gap-2">
                <Button
                  size="sm"
                  variant="outline"
                  onClick={() => handleAction('Screenshot', machine)}
                >
                  <Monitor className="h-3.5 w-3.5" />
                  Screenshot
                </Button>
                <Button
                  size="sm"
                  variant="outline"
                  onClick={() => handleAction('Logs', machine)}
                >
                  <FileText className="h-3.5 w-3.5" />
                  Logs
                </Button>
              </div>
            </TableCell>
          </TableRow>
        ))}
      </TableBody>
    </Table>
  )
}
