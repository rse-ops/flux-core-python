A NODESET is a comma separated list of integer ranks. Ranks may be
listed individually or as a range in the form *l-k* where l < k.

Some examples of nodesets.

\``1''
   rank 1

\``0-3''
   ranks 0, 1, 2, and 3 listed in a range

\``0,1,2,3''
   ranks 0, 1, 2, and 3 listed individually

\``2,5''
   ranks 2 and 5

\``2,4-5''
   ranks 2, 4, and 5

As a special case, the string \`\`\ *all*'' can be specified to indicate every
rank available in the flux instance.
