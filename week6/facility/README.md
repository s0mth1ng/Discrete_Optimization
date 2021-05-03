# Solution plan

## TODO
- [X] Default MIP solution. Cannot handle big tests.
- [ ] Local search solution:
  - [X] 1. Generate feasible solution using some heuristic (naive for now);
  - [ ] 2. Pick **randomly** one opened facility;
  - [ ] 3. Choose **randomly** other `k-1` facilities;
  - [ ] 4. Using MIP assign picked facilities to a customers;
  - [ ] 5. Repeat while improving;
  - [ ] 6. Enlarge `k`;
